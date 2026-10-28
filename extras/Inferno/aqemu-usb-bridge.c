/*
 * aqemu-usb-bridge: Inferno usb-tcp-remote replacement.
 *
 * Listens on 127.0.0.1:8030 (the address iOS QEMU already dials), speaks
 * Inferno's TCP USB protocol as the USB host, and attaches the other end of
 * a socketpair to the Linux vhci-hcd USB/IP port. usbmuxd and idevice*
 * then see the phone as a normal USB device. No companion VM.
 *
 * Must run as root so it can modprobe's sysfs attach node.
 */
#define _GNU_SOURCE
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#define TCP_USB_REQUEST 1
#define TCP_USB_RESPONSE 2
#define TCP_USB_RESET 3
#define TCP_USB_CANCEL 4

#define USB_TOKEN_SETUP 0x2d
#define USB_TOKEN_IN 0x69
#define USB_TOKEN_OUT 0xe1

#define USB_RET_SUCCESS 0
#define USB_RET_NODEV (-1)
#define USB_RET_NAK (-2)
#define USB_RET_STALL (-3)
#define USB_RET_BABBLE (-4)
#define USB_RET_IOERROR (-5)
#define USB_RET_ASYNC (-6)

#define USBIP_CMD_SUBMIT 0x0001
#define USBIP_CMD_UNLINK 0x0002
#define USBIP_RET_SUBMIT 0x0003
#define USBIP_RET_UNLINK 0x0004
#define USBIP_DIR_OUT 0
#define USBIP_DIR_IN 1

#define USB_REQ_SET_ADDRESS 5

#define VHCI_ATTACH "/sys/devices/platform/vhci_hcd.0/attach"
#define VHCI_DETACH "/sys/devices/platform/vhci_hcd.0/detach"
#define VHCI_PORT 0
#define VHCI_DEVID (((uint32_t)1 << 16) | 2)
#define USB_SPEED_HIGH 3
#define MAX_XFER (2 * 1024 * 1024)

struct tcp_req {
	uint8_t addr;
	int32_t pid;
	uint8_t ep;
	uint64_t id;
	uint32_t stream;
	uint8_t short_not_ok;
	uint8_t int_req;
	uint16_t length;
} __attribute__((packed));

struct tcp_rsp {
	uint8_t addr;
	int32_t pid;
	uint8_t ep;
	uint64_t id;
	uint32_t status;
	uint16_t length;
} __attribute__((packed));

struct tcp_can {
	uint8_t addr;
	int32_t pid;
	uint8_t ep;
	uint64_t id;
} __attribute__((packed));

_Static_assert(sizeof(struct tcp_req) == 22, "tcp request header");
_Static_assert(sizeof(struct tcp_rsp) == 20, "tcp response header");
_Static_assert(sizeof(struct tcp_can) == 14, "tcp cancel header");

struct waiter {
	uint64_t id;
	int done;
	int cancelled;
	int32_t status;
	uint16_t length;
	uint8_t *data;
	pthread_cond_t cv;
	struct waiter *next;
};

static int ios_fd = -1;
static int vhci_fd = -1;
static int kernel_fd = -1;
static int dead = 0;
static uint8_t dev_addr = 0;
static uint64_t next_id = 1;
static pthread_mutex_t mu = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t wr_mu = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t reply_mu = PTHREAD_MUTEX_INITIALIZER;
static struct waiter *waiters = NULL;

static void log_msg(const char *s)
{
	fprintf(stderr, "aqemu-usb-bridge: %s\n", s);
	fflush(stderr);
}

static int read_full(int fd, void *buf, size_t n)
{
	uint8_t *p = buf;
	size_t got = 0;
	while (got < n) {
		ssize_t r = read(fd, p + got, n - got);
		if (r == 0)
			return -1;
		if (r < 0) {
			if (errno == EINTR)
				continue;
			return -1;
		}
		got += (size_t)r;
	}
	return 0;
}

static int write_full(int fd, const void *buf, size_t n)
{
	const uint8_t *p = buf;
	size_t put = 0;
	while (put < n) {
		ssize_t r = write(fd, p + put, n - put);
		if (r < 0) {
			if (errno == EINTR)
				continue;
			return -1;
		}
		put += (size_t)r;
	}
	return 0;
}

static uint32_t get_be32(const uint8_t *p)
{
	return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
	       ((uint32_t)p[2] << 8) | (uint32_t)p[3];
}

static void put_be32(uint8_t *p, uint32_t v)
{
	p[0] = (uint8_t)(v >> 24);
	p[1] = (uint8_t)(v >> 16);
	p[2] = (uint8_t)(v >> 8);
	p[3] = (uint8_t)v;
}

static int map_status(int32_t st)
{
	if (st == USB_RET_SUCCESS)
		return 0;
	if (st == USB_RET_NAK)
		return -110; /* ETIMEDOUT: host retries */
	if (st == USB_RET_STALL)
		return -32; /* EPIPE */
	if (st == USB_RET_NODEV)
		return -19; /* ENODEV */
	if (st == USB_RET_BABBLE)
		return -75; /* EOVERFLOW */
	return -5; /* EIO */
}

static void fail_all_waiters(void)
{
	pthread_mutex_lock(&mu);
	dead = 1;
	for (struct waiter *w = waiters; w; w = w->next) {
		w->done = 1;
		w->status = USB_RET_NODEV;
		pthread_cond_signal(&w->cv);
	}
	pthread_mutex_unlock(&mu);
}

static void *reader_main(void *arg)
{
	(void)arg;
	for (;;) {
		uint8_t type = 0;
		if (read_full(ios_fd, &type, 1) != 0)
			break;
		if (type != TCP_USB_RESPONSE) {
			log_msg("iOS sent a non-response; closing");
			break;
		}
		struct tcp_rsp rsp;
		memset(&rsp, 0, sizeof rsp);
		if (read_full(ios_fd, &rsp, sizeof rsp) != 0)
			break;
		uint8_t *payload = NULL;
		if (rsp.length > 0 && rsp.pid == USB_TOKEN_IN &&
		    (int32_t)rsp.status != USB_RET_ASYNC) {
			if (rsp.length > MAX_XFER)
				break;
			payload = malloc(rsp.length);
			if (!payload || read_full(ios_fd, payload, rsp.length) != 0) {
				free(payload);
				break;
			}
		}
		if ((int32_t)rsp.status == USB_RET_ASYNC) {
			free(payload);
			continue;
		}
		pthread_mutex_lock(&mu);
		struct waiter **pp = &waiters;
		struct waiter *found = NULL;
		for (struct waiter *w = waiters; w; w = w->next) {
			if (w->id == rsp.id) {
				found = w;
				break;
			}
		}
		if (found) {
			found->status = (int32_t)rsp.status;
			found->length = rsp.length;
			found->data = payload;
			found->done = 1;
			pthread_cond_signal(&found->cv);
			payload = NULL;
		}
		(void)pp;
		pthread_mutex_unlock(&mu);
		free(payload);
	}
	fail_all_waiters();
	return NULL;
}

static int tcp_xfer(int32_t pid, uint8_t ep, int int_req, int short_not_ok,
		    const uint8_t *out, uint16_t length, uint8_t *in_buf,
		    uint16_t *actual, int timeout_ms)
{
	if (length > MAX_XFER)
		return USB_RET_IOERROR;
	struct waiter w;
	memset(&w, 0, sizeof w);
	pthread_cond_init(&w.cv, NULL);
	pthread_mutex_lock(&mu);
	if (dead) {
		pthread_mutex_unlock(&mu);
		pthread_cond_destroy(&w.cv);
		return USB_RET_NODEV;
	}
	w.id = next_id++;
	w.next = waiters;
	waiters = &w;
	uint64_t id = w.id;
	uint8_t addr = dev_addr;
	pthread_mutex_unlock(&mu);

	uint8_t type = TCP_USB_REQUEST;
	struct tcp_req req;
	memset(&req, 0, sizeof req);
	req.addr = addr;
	req.pid = pid;
	req.ep = ep;
	req.id = id;
	req.stream = 0;
	req.short_not_ok = (uint8_t)short_not_ok;
	req.int_req = (uint8_t)int_req;
	req.length = length;

	pthread_mutex_lock(&wr_mu);
	int wr = write_full(ios_fd, &type, 1) ||
		 write_full(ios_fd, &req, sizeof req) ||
		 (out && length ? write_full(ios_fd, out, length) : 0);
	pthread_mutex_unlock(&wr_mu);
	if (wr) {
		pthread_mutex_lock(&mu);
		dead = 1;
		w.done = 1;
		w.status = USB_RET_NODEV;
		pthread_mutex_unlock(&mu);
	}

	pthread_mutex_lock(&mu);
	if (!w.done && timeout_ms == 0) {
		while (!w.done && !w.cancelled)
			pthread_cond_wait(&w.cv, &mu);
	} else if (!w.done) {
		struct timespec ts;
		clock_gettime(CLOCK_REALTIME, &ts);
		ts.tv_sec += timeout_ms / 1000;
		ts.tv_nsec += (long)(timeout_ms % 1000) * 1000000L;
		if (ts.tv_nsec >= 1000000000L) {
			ts.tv_sec++;
			ts.tv_nsec -= 1000000000L;
		}
		while (!w.done && !w.cancelled) {
			if (pthread_cond_timedwait(&w.cv, &mu, &ts) == ETIMEDOUT)
				break;
		}
	}
	int32_t st = w.done ? w.status : USB_RET_IOERROR;
	uint16_t got = w.done ? w.length : 0;
	uint8_t *data = w.data;
	w.data = NULL;
	struct waiter **pp = &waiters;
	while (*pp) {
		if (*pp == &w) {
			*pp = w.next;
			break;
		}
		pp = &(*pp)->next;
	}
	int cancelled = w.cancelled;
	pthread_mutex_unlock(&mu);
	pthread_cond_destroy(&w.cv);

	if (cancelled && !w.done)
		st = USB_RET_IOERROR;
	if (in_buf && data && got)
		memcpy(in_buf, data, got < length ? got : length);
	free(data);
	if (actual)
		*actual = (st == USB_RET_SUCCESS) ? got : 0;
	return st;
}

static void send_reset(void)
{
	uint8_t type = TCP_USB_RESET;
	pthread_mutex_lock(&wr_mu);
	write_full(ios_fd, &type, 1);
	pthread_mutex_unlock(&wr_mu);
	dev_addr = 0;
}

static void send_cancel(int32_t pid, uint8_t ep, uint64_t id)
{
	uint8_t type = TCP_USB_CANCEL;
	struct tcp_can c;
	memset(&c, 0, sizeof c);
	c.addr = dev_addr;
	c.pid = pid;
	c.ep = ep;
	c.id = id;
	pthread_mutex_lock(&wr_mu);
	write_full(ios_fd, &type, 1);
	write_full(ios_fd, &c, sizeof c);
	pthread_mutex_unlock(&wr_mu);
}

struct job {
	uint32_t seq;
	uint32_t devid;
	uint32_t dir;
	uint32_t ep;
	int32_t length;
	int32_t interval;
	int32_t npkt;
	uint8_t setup[8];
	uint8_t *outbuf;
	int cancelled;
	struct job *next;
};

static struct job *jobs = NULL;
static pthread_mutex_t job_mu = PTHREAD_MUTEX_INITIALIZER;

static void reply_submit(uint32_t seq, uint32_t devid, uint32_t dir, uint32_t ep,
			 int status, const uint8_t *data, int32_t actual)
{
	uint8_t hdr[48];
	memset(hdr, 0, sizeof hdr);
	put_be32(hdr + 0, USBIP_RET_SUBMIT);
	put_be32(hdr + 4, seq);
	put_be32(hdr + 8, devid);
	put_be32(hdr + 12, dir);
	put_be32(hdr + 16, ep);
	put_be32(hdr + 20, (uint32_t)status);
	put_be32(hdr + 24, (uint32_t)(actual < 0 ? 0 : actual));
	pthread_mutex_lock(&reply_mu);
	write_full(vhci_fd, hdr, sizeof hdr);
	if (dir == USBIP_DIR_IN && actual > 0 && data)
		write_full(vhci_fd, data, (size_t)actual);
	pthread_mutex_unlock(&reply_mu);
}

static void reply_unlink(uint32_t seq, uint32_t devid, int status)
{
	uint8_t hdr[48];
	memset(hdr, 0, sizeof hdr);
	put_be32(hdr + 0, USBIP_RET_UNLINK);
	put_be32(hdr + 4, seq);
	put_be32(hdr + 8, devid);
	put_be32(hdr + 20, (uint32_t)status);
	pthread_mutex_lock(&reply_mu);
	write_full(vhci_fd, hdr, sizeof hdr);
	pthread_mutex_unlock(&reply_mu);
}

static int stage(int32_t pid, uint8_t ep, int int_req, const uint8_t *out,
		 uint16_t len, uint8_t *inb, uint16_t *actual, int timeout_ms,
		 int retries_on_nak)
{
	int st = USB_RET_NAK;
	for (int i = 0; i <= retries_on_nak; i++) {
		st = tcp_xfer(pid, ep, int_req, 0, out, len, inb, actual, timeout_ms);
		if (st != USB_RET_NAK)
			return st;
	}
	return st;
}

static void *job_main(void *arg)
{
	struct job *j = arg;
	int status = -5;
	uint8_t *inbuf = NULL;
	int32_t actual = 0;
	int int_req = j->interval != 0;

	if (j->npkt > 0) {
		status = -95; /* EOPNOTSUPP */
		goto out;
	}
	if (j->ep == 0) {
		uint16_t wLength = (uint16_t)(j->setup[6] | (j->setup[7] << 8));
		int is_in = (j->setup[0] & 0x80) != 0;
		/* SET_ADDRESS takes effect only after its status stage. */
		int set_addr = (j->setup[0] == 0 && j->setup[1] == USB_REQ_SET_ADDRESS);
		uint8_t new_addr = j->setup[2];
		uint16_t got = 0;
		int st = stage(USB_TOKEN_SETUP, 0, 0, j->setup, 8, NULL, &got, 5000, 4);
		if (st != USB_RET_SUCCESS) {
			status = map_status(st);
			goto out;
		}
		if (wLength > 0 && is_in) {
			if (wLength > MAX_XFER)
				wLength = MAX_XFER;
			inbuf = calloc(1, wLength ? wLength : 1);
			st = stage(USB_TOKEN_IN, 0, 0, NULL, wLength, inbuf, &got, 8000, 8);
			actual = got;
		} else if (wLength > 0 && !is_in) {
			uint16_t n = wLength;
			if (j->length >= 0 && (uint16_t)j->length < n)
				n = (uint16_t)j->length;
			st = stage(USB_TOKEN_OUT, 0, 0, j->outbuf, n, NULL, &got, 8000, 4);
			actual = (st == USB_RET_SUCCESS) ? (int32_t)n : 0;
		}
		if (st != USB_RET_SUCCESS) {
			status = map_status(st);
			goto out;
		}
		/* Status stage is the opposite direction of the data stage. */
		if (is_in || wLength == 0)
			st = stage(USB_TOKEN_OUT, 0, 0, NULL, 0, NULL, &got, 3000, 6);
		else
			st = stage(USB_TOKEN_IN, 0, 0, NULL, 0, NULL, &got, 3000, 6);
		status = map_status(st);
		if (status != 0)
			actual = 0;
		else if (set_addr) {
			pthread_mutex_lock(&mu);
			dev_addr = new_addr;
			pthread_mutex_unlock(&mu);
		}
	} else if (j->dir == USBIP_DIR_IN) {
		int32_t n = j->length < 0 ? 0 : j->length;
		if (n > MAX_XFER)
			n = MAX_XFER;
		inbuf = calloc(1, n ? (size_t)n : 1);
		int32_t filled = 0;
		int st = USB_RET_SUCCESS;
		/* Inferno's length field is 16-bit, so a larger URB is split. */
		while (filled < n && st == USB_RET_SUCCESS) {
			uint16_t chunk = (uint16_t)((n - filled) > 65535 ? 65535 : (n - filled));
			uint16_t got = 0;
			int timeout = int_req ? 30000 : 0;
			st = tcp_xfer(USB_TOKEN_IN, (uint8_t)j->ep, int_req, 0,
				      NULL, chunk, inbuf + filled, &got, timeout);
			if (st == USB_RET_SUCCESS) {
				filled += got;
				if (got < chunk)
					break;
			}
		}
		status = map_status(st);
		actual = (status == 0) ? filled : 0;
	} else {
		int32_t n = j->length < 0 ? 0 : j->length;
		if (n > MAX_XFER)
			n = MAX_XFER;
		int32_t sent = 0;
		int st = USB_RET_SUCCESS;
		while (sent < n && st == USB_RET_SUCCESS) {
			uint16_t chunk = (uint16_t)((n - sent) > 65535 ? 65535 : (n - sent));
			uint16_t got = 0;
			st = tcp_xfer(USB_TOKEN_OUT, (uint8_t)j->ep, int_req, 0,
				      j->outbuf + sent, chunk, NULL, &got, 15000);
			if (st == USB_RET_SUCCESS)
				sent += chunk;
		}
		status = map_status(st);
		actual = (status == 0) ? sent : 0;
	}
out:
	pthread_mutex_lock(&job_mu);
	int cancelled = j->cancelled;
	pthread_mutex_unlock(&job_mu);
	if (!cancelled)
		reply_submit(j->seq, j->devid, j->dir, j->ep, status, inbuf, actual);
	free(inbuf);
	free(j->outbuf);
	pthread_mutex_lock(&job_mu);
	struct job **pp = &jobs;
	while (*pp) {
		if (*pp == j) {
			*pp = j->next;
			break;
		}
		pp = &(*pp)->next;
	}
	pthread_mutex_unlock(&job_mu);
	free(j);
	return NULL;
}

static void cancel_seq(uint32_t seq)
{
	pthread_mutex_lock(&job_mu);
	for (struct job *j = jobs; j; j = j->next) {
		if (j->seq == seq)
			j->cancelled = 1;
	}
	pthread_mutex_unlock(&job_mu);
	pthread_mutex_lock(&mu);
	for (struct waiter *w = waiters; w; w = w->next) {
		w->cancelled = 1;
		pthread_cond_signal(&w->cv);
	}
	pthread_mutex_unlock(&mu);
}

static void *command_main(void *arg)
{
	(void)arg;
	for (;;) {
		uint8_t hdr[48];
		if (read_full(vhci_fd, hdr, sizeof hdr) != 0)
			break;
		uint32_t cmd = get_be32(hdr + 0);
		uint32_t seq = get_be32(hdr + 4);
		uint32_t devid = get_be32(hdr + 8);
		uint32_t dir = get_be32(hdr + 12);
		uint32_t ep = get_be32(hdr + 16);
		if (cmd == USBIP_CMD_UNLINK) {
			uint32_t target = get_be32(hdr + 20);
			cancel_seq(target);
			reply_unlink(seq, devid, 0);
			continue;
		}
		if (cmd != USBIP_CMD_SUBMIT) {
			log_msg("unknown usbip command");
			break;
		}
		int32_t length = (int32_t)get_be32(hdr + 24);
		int32_t npkt = (int32_t)get_be32(hdr + 32);
		int32_t interval = (int32_t)get_be32(hdr + 36);
		if (dead)
			break;
		uint8_t *outbuf = NULL;
		if (dir == USBIP_DIR_OUT && length > 0) {
			if (length > MAX_XFER)
				break;
			outbuf = malloc((size_t)length);
			if (!outbuf || read_full(vhci_fd, outbuf, (size_t)length) != 0) {
				free(outbuf);
				break;
			}
		}
		if (npkt > 0) {
			/* Iso descriptors follow the OUT buffer. */
			if (npkt > 1024) {
				free(outbuf);
				break;
			}
			uint8_t junk[16];
			for (int i = 0; i < npkt; i++) {
				if (read_full(vhci_fd, junk, sizeof junk) != 0) {
					free(outbuf);
					goto done;
				}
			}
		}
		struct job *j = calloc(1, sizeof *j);
		if (!j) {
			free(outbuf);
			break;
		}
		j->seq = seq;
		j->devid = devid;
		j->dir = dir;
		j->ep = ep;
		j->length = length;
		j->interval = interval;
		j->npkt = npkt;
		memcpy(j->setup, hdr + 40, 8);
		j->outbuf = outbuf;
		pthread_mutex_lock(&job_mu);
		j->next = jobs;
		jobs = j;
		pthread_mutex_unlock(&job_mu);
		pthread_t th;
		if (pthread_create(&th, NULL, job_main, j) != 0) {
			reply_submit(seq, devid, dir, ep, -5, NULL, 0);
			pthread_mutex_lock(&job_mu);
			jobs = j->next;
			pthread_mutex_unlock(&job_mu);
			free(outbuf);
			free(j);
			continue;
		}
		pthread_detach(th);
	}
done:
	fail_all_waiters();
	return NULL;
}

static int attach_vhci(void)
{
	int sv[2];
	if (socketpair(AF_UNIX, SOCK_STREAM, 0, sv) != 0) {
		perror("socketpair");
		return -1;
	}
	int snd = 4 * 1024 * 1024;
	setsockopt(sv[0], SOL_SOCKET, SO_SNDBUF, &snd, sizeof snd);
	setsockopt(sv[1], SOL_SOCKET, SO_SNDBUF, &snd, sizeof snd);
	setsockopt(sv[0], SOL_SOCKET, SO_RCVBUF, &snd, sizeof snd);
	setsockopt(sv[1], SOL_SOCKET, SO_RCVBUF, &snd, sizeof snd);
	kernel_fd = sv[0];
	vhci_fd = sv[1];
	int fd = open(VHCI_ATTACH, O_WRONLY);
	if (fd < 0) {
		perror(VHCI_ATTACH);
		log_msg("modprobe vhci-hcd first");
		return -1;
	}
	char line[64];
	snprintf(line, sizeof line, "%u %u %u %u\n", VHCI_PORT, (unsigned)kernel_fd,
		 VHCI_DEVID, USB_SPEED_HIGH);
	ssize_t n = write(fd, line, strlen(line));
	int err = errno;
	close(fd);
	if (n < 0) {
		errno = err;
		perror("vhci attach");
		return -1;
	}
	log_msg("attached to vhci-hcd port 0");
	return 0;
}

static void detach_vhci(void)
{
	int fd = open(VHCI_DETACH, O_WRONLY);
	if (fd >= 0) {
		char line[32];
		snprintf(line, sizeof line, "%u\n", VHCI_PORT);
		write(fd, line, strlen(line));
		close(fd);
	}
	if (kernel_fd >= 0)
		close(kernel_fd);
	if (vhci_fd >= 0)
		close(vhci_fd);
	kernel_fd = -1;
	vhci_fd = -1;
}

static void serve(int client)
{
	ios_fd = client;
	dead = 0;
	dev_addr = 0;
	int one = 1;
	setsockopt(client, IPPROTO_TCP, TCP_NODELAY, &one, sizeof one);
	pthread_t reader;
	if (pthread_create(&reader, NULL, reader_main, NULL) != 0) {
		log_msg("reader thread failed");
		return;
	}
	send_reset();
	if (attach_vhci() != 0) {
		fail_all_waiters();
		pthread_join(reader, NULL);
		return;
	}
	pthread_t commands;
	if (pthread_create(&commands, NULL, command_main, NULL) != 0) {
		detach_vhci();
		fail_all_waiters();
		pthread_join(reader, NULL);
		return;
	}
	pthread_join(commands, NULL);
	detach_vhci();
	shutdown(client, SHUT_RDWR);
	pthread_join(reader, NULL);
	log_msg("iOS USB link closed");
}

int main(int argc, char **argv)
{
	if (argc > 1 && strcmp(argv[1], "--self-check") == 0) {
		printf("aqemu-usb-bridge struct check ok\n");
		return 0;
	}
	int port = 8030;
	if (argc > 1)
		port = atoi(argv[1]);
	if (port <= 0 || port > 65535)
		port = 8030;
	if (geteuid() != 0) {
		log_msg("must run as root to attach vhci-hcd");
		return 1;
	}
	signal(SIGPIPE, SIG_IGN);
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0) {
		perror("socket");
		return 1;
	}
	int one = 1;
	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof one);
	struct sockaddr_in addr;
	memset(&addr, 0, sizeof addr);
	addr.sin_family = AF_INET;
	addr.sin_port = htons((uint16_t)port);
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	if (bind(fd, (struct sockaddr *)&addr, sizeof addr) != 0) {
		fprintf(stderr, "aqemu-usb-bridge: bind 127.0.0.1:%d failed: %s\n",
			port, strerror(errno));
		fprintf(stderr, "aqemu-usb-bridge: stop the companion if it still listens on this port\n");
		return 1;
	}
	if (listen(fd, 1) != 0) {
		perror("listen");
		return 1;
	}
	fprintf(stderr, "aqemu-usb-bridge: listening on 127.0.0.1:%d\n", port);
	fflush(stderr);
	for (;;) {
		int c = accept(fd, NULL, NULL);
		if (c < 0) {
			if (errno == EINTR)
				continue;
			perror("accept");
			return 1;
		}
		log_msg("iOS connected");
		serve(c);
		close(c);
		ios_fd = -1;
	}
}
