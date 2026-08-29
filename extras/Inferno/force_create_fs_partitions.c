#define _GNU_SOURCE
#include <fcntl.h>
#include <dlfcn.h>
#include <elf.h>
#include <errno.h>
#include <link.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <stdarg.h>

static void aqemu_log(const char *fmt, ...)
{
	va_list ap;
	FILE *f;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	fflush(stderr);
	f = fopen("/tmp/aqemu-cfs.log", "a");
	if( f ) {
		va_start(ap, fmt);
		vfprintf(f, fmt, ap);
		va_end(ap);
		fflush(f);
		fclose(f);
	}
}

typedef void *plist_t;
typedef void *restored_client_t;
typedef int restored_error_t;

void plist_dict_set_item(plist_t dict, const char *key, plist_t value);
restored_error_t restored_start_restore(restored_client_t client, plist_t options, uint64_t version);
restored_error_t restored_send(restored_client_t client, plist_t plist);
static int aqemu_mprotect_rwx(void *p, size_t n)
{
	long ps = sysconf(_SC_PAGESIZE);
	uintptr_t start, end;
	if( ps <= 0 )
		ps = 4096;
	start = (uintptr_t)p & ~((uintptr_t)ps - 1);
	end = ((uintptr_t)p + n + (uintptr_t)ps - 1) & ~((uintptr_t)ps - 1);
	return mprotect((void *)start, (size_t)( end - start ),
		PROT_READ | PROT_WRITE | PROT_EXEC);
}

#define AQEMU_JMP 16
static unsigned char aqemu_rsr_saved[AQEMU_JMP];
static void *aqemu_rsr_real;
static int aqemu_rsr_hooked;
static unsigned char aqemu_p2b_saved[AQEMU_JMP];
static void *aqemu_p2b_real;
static int aqemu_p2b_hooked;
static unsigned char aqemu_p2x_saved[AQEMU_JMP];
static void *aqemu_p2x_real;
static int aqemu_p2x_hooked;
static unsigned char aqemu_rsd_saved[AQEMU_JMP];
static void *aqemu_rsd_real;
static int aqemu_rsd_hooked;

void plist_to_bin(plist_t plist, char **bin, uint32_t *length);
void plist_to_xml(plist_t plist, char **xml, uint32_t *length);

static void aqemu_install_jmp(void *from, void *to)
{
	unsigned char *p = (unsigned char *)from;
	/* endbr64 so IBT/CET still accepts the original function address */
	p[0] = 0xF3;
	p[1] = 0x0F;
	p[2] = 0x1E;
	p[3] = 0xFA;
	p[4] = 0x48;
	p[5] = 0xB8;
	memcpy(p + 6, &to, 8);
	p[14] = 0xFF;
	p[15] = 0xE0;
	__builtin___clear_cache((char *)from, (char *)from + AQEMU_JMP);
}

static restored_error_t aqemu_call_real_rsr(restored_client_t client, plist_t options, uint64_t version)
{
	typedef restored_error_t (*fn)(restored_client_t, plist_t, uint64_t);
	restored_error_t r;
	memcpy(aqemu_rsr_real, aqemu_rsr_saved, AQEMU_JMP);
	r = ((fn)aqemu_rsr_real)(client, options, version);
	aqemu_install_jmp(aqemu_rsr_real, (void *)restored_start_restore);
	return r;
}

static void aqemu_inline_hook_rsr(void)
{
	aqemu_rsr_real = dlsym(RTLD_NEXT, "restored_start_restore");
	if( ! aqemu_rsr_real || aqemu_rsr_real == (void *)restored_start_restore ) {
		fprintf(stderr, "AQEMU: no libimobiledevice restored_start_restore to inline-hook\n");
		return;
	}
	if( aqemu_mprotect_rwx(aqemu_rsr_real, AQEMU_JMP) != 0 ) {
		fprintf(stderr, "AQEMU: mprotect restored_start_restore failed errno=%d\n", errno);
		return;
	}
	memcpy(aqemu_rsr_saved, aqemu_rsr_real, AQEMU_JMP);
	aqemu_install_jmp(aqemu_rsr_real, (void *)restored_start_restore);
	aqemu_rsr_hooked = 1;
	fprintf(stderr, "AQEMU: inline hooked restored_start_restore at %p bytes %02x %02x %02x %02x\n",
		aqemu_rsr_real,
		((unsigned char *)aqemu_rsr_real)[0],
		((unsigned char *)aqemu_rsr_real)[1],
		((unsigned char *)aqemu_rsr_real)[2],
		((unsigned char *)aqemu_rsr_real)[3]);
}

static void aqemu_inline_hook_p2b(void)
{
	aqemu_p2b_real = dlsym(RTLD_NEXT, "plist_to_bin");
	if( ! aqemu_p2b_real || aqemu_p2b_real == (void *)plist_to_bin ) {
		fprintf(stderr, "AQEMU: no libplist plist_to_bin to inline-hook\n");
		return;
	}
	if( aqemu_mprotect_rwx(aqemu_p2b_real, AQEMU_JMP) != 0 ) {
		fprintf(stderr, "AQEMU: mprotect plist_to_bin failed errno=%d\n", errno);
		return;
	}
	memcpy(aqemu_p2b_saved, aqemu_p2b_real, AQEMU_JMP);
	aqemu_install_jmp(aqemu_p2b_real, (void *)plist_to_bin);
	aqemu_p2b_hooked = 1;
	fprintf(stderr, "AQEMU: inline hooked plist_to_bin at %p\n", aqemu_p2b_real);
}

static void aqemu_inline_hook_p2x(void)
{
	aqemu_p2x_real = dlsym(RTLD_NEXT, "plist_to_xml");
	if( ! aqemu_p2x_real || aqemu_p2x_real == (void *)plist_to_xml ) {
		aqemu_p2x_real = dlvsym(RTLD_NEXT, "plist_to_xml", "LIBPLIST_2.0");
		if( ! aqemu_p2x_real || aqemu_p2x_real == (void *)plist_to_xml )
			fprintf(stderr, "AQEMU: no libplist plist_to_xml to inline-hook\n");
	}
	if( ! aqemu_p2x_real || aqemu_p2x_real == (void *)plist_to_xml )
		return;
	if( aqemu_mprotect_rwx(aqemu_p2x_real, AQEMU_JMP) != 0 ) {
		fprintf(stderr, "AQEMU: mprotect plist_to_xml failed errno=%d\n", errno);
		return;
	}
	memcpy(aqemu_p2x_saved, aqemu_p2x_real, AQEMU_JMP);
	aqemu_install_jmp(aqemu_p2x_real, (void *)plist_to_xml);
	aqemu_p2x_hooked = 1;
	fprintf(stderr, "AQEMU: inline hooked plist_to_xml at %p bytes %02x %02x %02x %02x\n",
		aqemu_p2x_real,
		((unsigned char *)aqemu_p2x_real)[0],
		((unsigned char *)aqemu_p2x_real)[1],
		((unsigned char *)aqemu_p2x_real)[2],
		((unsigned char *)aqemu_p2x_real)[3]);
}

static restored_error_t aqemu_call_real_rsd(restored_client_t client, plist_t plist)
{
	typedef restored_error_t (*fn)(restored_client_t, plist_t);
	restored_error_t r;
	memcpy(aqemu_rsd_real, aqemu_rsd_saved, AQEMU_JMP);
	r = ((fn)aqemu_rsd_real)(client, plist);
	aqemu_install_jmp(aqemu_rsd_real, (void *)restored_send);
	return r;
}

static void aqemu_inline_hook_rsd(void)
{
	aqemu_rsd_real = dlsym(RTLD_NEXT, "restored_send");
	if( ! aqemu_rsd_real || aqemu_rsd_real == (void *)restored_send ) {
		aqemu_rsd_real = dlvsym(RTLD_NEXT, "restored_send", "LIBIMOBILEDEVICE_1.0");
		if( ! aqemu_rsd_real || aqemu_rsd_real == (void *)restored_send )
			fprintf(stderr, "AQEMU: no libimobiledevice restored_send to inline-hook\n");
	}
	if( ! aqemu_rsd_real || aqemu_rsd_real == (void *)restored_send )
		return;
	if( aqemu_mprotect_rwx(aqemu_rsd_real, AQEMU_JMP) != 0 ) {
		fprintf(stderr, "AQEMU: mprotect restored_send failed errno=%d\n", errno);
		return;
	}
	memcpy(aqemu_rsd_saved, aqemu_rsd_real, AQEMU_JMP);
	aqemu_install_jmp(aqemu_rsd_real, (void *)restored_send);
	aqemu_rsd_hooked = 1;
	fprintf(stderr, "AQEMU: inline hooked restored_send at %p\n", aqemu_rsd_real);
}

static void aqemu_got_rewire(void);
static void aqemu_patch_exe_got(void);

__attribute__((constructor))
static void aqemu_cfs_init(void)
{
	unlink("/tmp/aqemu-cfs.log");
	aqemu_log("AQEMU: force_cfs.so loaded (exe GOT + plist_to_xml + restored_start_restore)\n");
	aqemu_log("AQEMU: our restored_start_restore=%p plist_to_xml=%p plist_dict_set_item=%p restored_send=%p\n",
		(void *)restored_start_restore, (void *)plist_to_xml, (void *)plist_dict_set_item,
		(void *)restored_send);
	aqemu_log("AQEMU: plist_dict_set_item next=%p restored_start_restore next=%p SSL_write next=%p plist_to_xml next=%p restored_send next=%p\n",
		dlsym(RTLD_NEXT, "plist_dict_set_item"),
		dlsym(RTLD_NEXT, "restored_start_restore"),
		dlsym(RTLD_NEXT, "SSL_write"),
		dlsym(RTLD_NEXT, "plist_to_xml"),
		dlsym(RTLD_NEXT, "restored_send"));
	aqemu_got_rewire();
	aqemu_patch_exe_got();
	aqemu_inline_hook_rsr();
	aqemu_inline_hook_rsd();
	aqemu_inline_hook_p2b();
	aqemu_inline_hook_p2x();
}

static void *symver(const char *name)
{
	static const char *vers[] = {
		"LIBPLIST_2.0", "LIBPLIST_2.2", "LIBPLIST_2.3",
		"LIBIMOBILEDEVICE_1.0", "LIBIMOBILEDEVICE_1.0.6",
		"OPENSSL_3.0.0", "OPENSSL_3.1.0", "OPENSSL_3.2.0", "OPENSSL_3.3.0",
		"OPENSSL_3.4.0", "OPENSSL_3.0.2",
		NULL
	};
	void *p = dlsym(RTLD_NEXT, name);
	int i;
	if( p )
		return p;
	for( i = 0; vers[i]; i++ ) {
		p = dlvsym(RTLD_NEXT, name, vers[i]);
		if( p )
			return p;
	}
	return NULL;
}

static uint64_t rd_be(const unsigned char *p, int n)
{
	uint64_t v = 0;
	int i;
	for( i = 0; i < n; i++ )
		v = ( v << 8 ) | p[i];
	return v;
}

static int patch_xml(unsigned char *buf, size_t n)
{
	unsigned char *key = memmem(buf, n, "CreateFilesystemPartitions", 26);
	unsigned char *p, *end, *f;
	if( ! key )
		return 0;
	p = key + 26;
	end = buf + n;
	if( end - p > 512 )
		end = p + 512;
	f = memmem(p, (size_t)( end - p ), "<false", 6);
	if( ! f )
		return 0;
	if( f + 8 <= buf + n && memcmp(f, "<false/>", 8) == 0 )
		memcpy(f, "<true/> ", 8);
	else if( f + 15 <= buf + n && memcmp(f, "<false></false>", 15) == 0 )
		memcpy(f, "<true></true>  ", 15);
	else
		memcpy(f + 1, "true ", 5);
	fprintf(stderr, "AQEMU: patched XML CreateFilesystemPartitions to true\n");
	return 1;
}

static int patch_bplist(unsigned char *buf, size_t n)
{
	const unsigned char *tr;
	int off_sz, ref_sz;
	uint64_t num_obj, top, off_tab, i, k, nent, o, tlen;
	unsigned char *obj, *p, *end, marker;
	int low;
	const char *want = "CreateFilesystemPartitions";
	const size_t want_len = 26;

	if( n < 40 || memcmp(buf, "bplist00", 8) != 0 )
		return 0;
	tr = buf + n - 32;
	off_sz = tr[6];
	ref_sz = tr[7];
	num_obj = rd_be(tr + 8, 8);
	top = rd_be(tr + 16, 8);
	off_tab = rd_be(tr + 24, 8);
	if( off_sz < 1 || off_sz > 8 || ref_sz < 1 || ref_sz > 8 || num_obj < 1 || num_obj > 100000 )
		return 0;
	if( off_tab + num_obj * (uint64_t)off_sz > n )
		return 0;
	end = buf + n;

	if( top >= num_obj )
		return 0;
	o = rd_be(buf + off_tab + top * (uint64_t)off_sz, off_sz);
	if( o >= n )
		return 0;
	obj = buf + o;
	marker = *obj;
	if( ( marker & 0xF0 ) != 0xD0 )
		return 0;
	p = obj + 1;
	low = marker & 0x0F;
	nent = (uint64_t)low;
	if( low == 0x0F ) {
		if( p >= end )
			return 0;
		marker = *p++;
		if( ( marker & 0xF0 ) != 0x10 )
			return 0;
		tlen = (uint64_t)( 1 << ( marker & 0x0F ) );
		if( p + tlen > end )
			return 0;
		nent = rd_be(p, (int)tlen);
		p += tlen;
	}
	if( nent > 4096 || p + nent * 2 * (uint64_t)ref_sz > end )
		return 0;

	for( i = 0; i < nent; i++ ) {
		uint64_t key_i = rd_be(p + i * (uint64_t)ref_sz, ref_sz);
		uint64_t val_i = rd_be(p + ( nent + i ) * (uint64_t)ref_sz, ref_sz);
		uint64_t ko, vo;
		unsigned char *ks, *vs, *sp;
		uint64_t slen;
		int slow;
		if( key_i >= num_obj || val_i >= num_obj )
			continue;
		ko = rd_be(buf + off_tab + key_i * (uint64_t)off_sz, off_sz);
		vo = rd_be(buf + off_tab + val_i * (uint64_t)off_sz, off_sz);
		if( ko >= n || vo >= n )
			continue;
		ks = buf + ko;
		if( ( *ks & 0xF0 ) != 0x50 )
			continue;
		sp = ks + 1;
		slow = *ks & 0x0F;
		slen = (uint64_t)slow;
		if( slow == 0x0F ) {
			if( sp >= end )
				continue;
			marker = *sp++;
			if( ( marker & 0xF0 ) != 0x10 )
				continue;
			tlen = (uint64_t)( 1 << ( marker & 0x0F ) );
			if( sp + tlen > end )
				continue;
			slen = rd_be(sp, (int)tlen);
			sp += tlen;
		}
		if( slen != want_len || sp + slen > end )
			continue;
		if( memcmp(sp, want, want_len) != 0 )
			continue;
		vs = buf + vo;
		if( *vs == 0x08 || *vs == 0x09 ) {
			*vs = 0x09;
			fprintf(stderr, "AQEMU: patched bplist CreateFilesystemPartitions to true\n");
			return 1;
		}
		for( k = 0; k < 8 && vs + k < end; k++ ) {
			if( vs[k] == 0x08 ) {
				vs[k] = 0x09;
				fprintf(stderr, "AQEMU: patched bplist bool near CreateFilesystemPartitions\n");
				return 1;
			}
		}
	}
	return 0;
}

static void patch_payload(void *buf, size_t n)
{
	if( ! buf || n < 20 )
		return;
	if( ! memmem(buf, n, "CreateFilesystemPartitions", 26) ) {
		if( memmem(buf, n, "PersonalizedDuringPreflight", 27) ||
		    memmem(buf, n, "StartRestore", 12) )
			fprintf(stderr,
				"AQEMU: restore plist (%zu bytes) has no CreateFilesystemPartitions key\n",
				n);
		return;
	}
	if( patch_xml(buf, n) )
		return;
	if( patch_bplist(buf, n) )
		return;
	fprintf(stderr,
		"AQEMU: CreateFilesystemPartitions present (%zu bytes) but XML/bplist rewrite missed\n",
		n);
}

static int buf_is_restore_wire(const void *buf, size_t n)
{
	if( ! buf || n < 12 )
		return 0;
	if( memmem(buf, n, "CreateFilesystemPartitions", 26) )
		return 1;
	if( memmem(buf, n, "StartRestore", 12) )
		return 1;
	if( memmem(buf, n, "PersonalizedDuringPreflight", 27) )
		return 1;
	if( memmem(buf, n, "RestoreOptions", 14) )
		return 1;
	return 0;
}

static void notice_restore_plist(const void *buf, size_t n)
{
	static int once;
	if( once || ! buf || n < 40 )
		return;
	if( ! memmem(buf, n, "PersonalizedDuringPreflight", 27) &&
	    ! memmem(buf, n, "StartRestore", 12) &&
	    ! memmem(buf, n, "CreateFilesystemPartitions", 26) )
		return;
	once = 1;
	fprintf(stderr, "AQEMU: intercept restore-related payload (%zu bytes)\n", n);
	if( ! memmem(buf, n, "CreateFilesystemPartitions", 26) )
		fprintf(stderr, "AQEMU: that payload has no CreateFilesystemPartitions key\n");
}

static void *copy_patch(const void *buf, size_t n)
{
	void *copy;
	notice_restore_plist(buf, n);
	if( ! buf || n < 20 || ! buf_is_restore_wire(buf, n) )
		return NULL;
	copy = malloc(n);
	if( ! copy )
		return NULL;
	memcpy(copy, buf, n);
	patch_payload(copy, n);
	return copy;
}

int SSL_write(void *ssl, const void *buf, int num)
{
	static int (*real)(void *, const void *, int);
	void *copy;
	int r;
	if( ! real )
		real = (int (*)(void *, const void *, int))symver("SSL_write");
	if( ! real )
		real = (int (*)(void *, const void *, int))dlsym(RTLD_NEXT, "SSL_write");
	if( ! real )
		return -1;
	{
		static int nlog;
		if( nlog < 3 ) {
			nlog++;
			fprintf(stderr, "AQEMU: SSL_write payload %d bytes\n", num);
		}
	}
	copy = ( num > 0 ) ? copy_patch(buf, (size_t)num) : NULL;
	if( copy ) {
		r = real(ssl, copy, num);
		free(copy);
		return r;
	}
	return real(ssl, buf, num);
}

int SSL_write_ex(void *ssl, const void *buf, size_t num, size_t *written)
{
	static int (*real)(void *, const void *, size_t, size_t *);
	void *copy;
	int r;
	if( ! real )
		real = (int (*)(void *, const void *, size_t, size_t *))dlsym(RTLD_NEXT, "SSL_write_ex");
	if( ! real )
		return 0;
	if( num > 64 ) {
		static int once;
		if( ! once ) {
			once = 1;
			fprintf(stderr, "AQEMU: SSL_write_ex first payload %zu bytes\n", num);
		}
	}
	copy = copy_patch(buf, num);
	if( copy ) {
		r = real(ssl, copy, num, written);
		free(copy);
		return r;
	}
	return real(ssl, buf, num, written);
}

int idevice_connection_send(void *conn, const char *data, uint32_t len, uint32_t *sent)
{
	static int (*real)(void *, const char *, uint32_t, uint32_t *);
	void *copy;
	int r;
	if( ! real )
		real = (int (*)(void *, const char *, uint32_t, uint32_t *))
			dlsym(RTLD_NEXT, "idevice_connection_send");
	if( ! real || real == idevice_connection_send )
		real = (int (*)(void *, const char *, uint32_t, uint32_t *))
			dlsym(RTLD_DEFAULT, "idevice_connection_send");
	if( ! real || real == idevice_connection_send )
		return -1;
	if( len > 64 ) {
		static int once;
		if( ! once ) {
			once = 1;
			fprintf(stderr, "AQEMU: idevice_connection_send first payload %u bytes\n", len);
		}
	}
	copy = copy_patch(data, (size_t)len);
	if( copy ) {
		r = real(conn, (const char *)copy, len, sent);
		free(copy);
		return r;
	}
	return real(conn, data, len, sent);
}

ssize_t gnutls_record_send(void *session, const void *data, size_t sizeofdata)
{
	static ssize_t (*real)(void *, const void *, size_t);
	void *copy;
	ssize_t r;
	if( ! real )
		real = (ssize_t (*)(void *, const void *, size_t))dlsym(RTLD_NEXT, "gnutls_record_send");
	if( ! real )
		return -1;
	copy = copy_patch(data, sizeofdata);
	if( copy ) {
		r = real(session, copy, sizeofdata);
		free(copy);
		return r;
	}
	return real(session, data, sizeofdata);
}

static int drop_ios18_key(const char *key)
{
	return key && (
		strcmp(key, "SystemImageFormat") == 0 ||
		strcmp(key, "HostHasFixFor99053849") == 0 ||
		strcmp(key, "WaitForDeviceConnectionToFinishStateMachine") == 0 ||
		strcmp(key, "SupportedAsyncDataTypes") == 0);
}

static void force_true(plist_t dict)
{
	typedef plist_t (*new_bool_fn)(uint8_t);
	typedef plist_t (*new_str_fn)(const char *);
	new_bool_fn plist_new_bool = (new_bool_fn)symver("plist_new_bool");
	new_str_fn plist_new_string;
	if( ! plist_new_bool )
		plist_new_bool = (new_bool_fn)dlsym(RTLD_DEFAULT, "plist_new_bool");
	aqemu_log("AQEMU: force_true dict=%p plist_new_bool=%p\n", dict, (void *)plist_new_bool);
	if( ! dict || ! plist_new_bool ) {
		aqemu_log("AQEMU: force_true abort (need dict and plist_new_bool)\n");
		return;
	}
	aqemu_log("AQEMU: forcing CreateFilesystemPartitions=true + AuthInstallRestoreBehavior=Erase\n");
	plist_dict_set_item(dict, "CreateFilesystemPartitions", plist_new_bool(1));
	plist_new_string = (new_str_fn)symver("plist_new_string");
	if( ! plist_new_string )
		plist_new_string = (new_str_fn)dlsym(RTLD_DEFAULT, "plist_new_string");
	if( plist_new_string )
		plist_dict_set_item(dict, "AuthInstallRestoreBehavior", plist_new_string("Erase"));
}

void plist_dict_set_item(plist_t dict, const char *key, plist_t value)
{
	typedef void (*fn)(plist_t, const char *, plist_t);
	static fn real;
	if( ! real )
		real = (fn)dlsym(RTLD_NEXT, "plist_dict_set_item");
	if( drop_ios18_key(key) ) {
		fprintf(stderr, "AQEMU: dropping iOS18 restore option %s\n", key);
		return;
	}
	if( key && strcmp(key, "CreateFilesystemPartitions") == 0 ) {
		typedef plist_t (*new_bool_fn)(uint8_t);
		new_bool_fn plist_new_bool = (new_bool_fn)symver("plist_new_bool");
		if( plist_new_bool )
			value = plist_new_bool(1);
		fprintf(stderr, "AQEMU: CreateFilesystemPartitions -> true\n");
	}
	if( real )
		real(dict, key, value);
}

void plist_dict_insert_item(plist_t dict, const char *key, plist_t value)
{
	typedef void (*fn)(plist_t, const char *, plist_t);
	static fn real;
	if( ! real )
		real = (fn)dlsym(RTLD_NEXT, "plist_dict_insert_item");
	if( key && strcmp(key, "CreateFilesystemPartitions") == 0 ) {
		typedef plist_t (*new_bool_fn)(uint8_t);
		new_bool_fn plist_new_bool = (new_bool_fn)symver("plist_new_bool");
		if( plist_new_bool )
			value = plist_new_bool(1);
		fprintf(stderr, "AQEMU: CreateFilesystemPartitions (insert) -> true\n");
	}
	if( real )
		real(dict, key, value);
}

void plist_to_xml(plist_t plist, char **xml, uint32_t *length)
{
	typedef void (*fn)(plist_t, char **, uint32_t *);
	static fn real;
	static int nlog;
	if( nlog < 8 ) {
		nlog++;
		aqemu_log("AQEMU: plist_to_xml ENTER #%d hooked=%d\n", nlog, aqemu_p2x_hooked);
	}
	if( aqemu_p2x_hooked && aqemu_p2x_real ) {
		memcpy(aqemu_p2x_real, aqemu_p2x_saved, AQEMU_JMP);
		((fn)aqemu_p2x_real)(plist, xml, length);
		aqemu_install_jmp(aqemu_p2x_real, (void *)plist_to_xml);
		if( xml && *xml && length && *length )
			patch_payload(*xml, *length);
		return;
	}
	if( ! real )
		real = (fn)symver("plist_to_xml");
	if( ! real )
		return;
	real(plist, xml, length);
	if( xml && *xml && length && *length )
		patch_payload(*xml, *length);
}

void plist_to_bin(plist_t plist, char **bin, uint32_t *length)
{
	typedef void (*fn)(plist_t, char **, uint32_t *);
	static fn real;
	static int nlog;
	if( nlog < 8 ) {
		nlog++;
		aqemu_log("AQEMU: plist_to_bin ENTER #%d hooked=%d\n", nlog, aqemu_p2b_hooked);
	}
	if( aqemu_p2b_hooked && aqemu_p2b_real ) {
		memcpy(aqemu_p2b_real, aqemu_p2b_saved, AQEMU_JMP);
		((fn)aqemu_p2b_real)(plist, bin, length);
		aqemu_install_jmp(aqemu_p2b_real, (void *)plist_to_bin);
		if( bin && *bin && length && *length )
			patch_payload(*bin, *length);
		return;
	}
	if( ! real )
		real = (fn)symver("plist_to_bin");
	if( ! real )
		return;
	real(plist, bin, length);
	if( bin && *bin && length && *length )
		patch_payload(*bin, *length);
}

restored_error_t restored_start_restore(restored_client_t client, plist_t options, uint64_t version)
{
	typedef restored_error_t (*fn)(restored_client_t, plist_t, uint64_t);
	static fn real;
	aqemu_log("AQEMU: restored_start_restore ENTER dict=%p hooked=%d\n",
		options, aqemu_rsr_hooked);
	force_true(options);
	{
		char *xml = NULL;
		uint32_t xlen = 0;
		typedef void (*px)(plist_t, char **, uint32_t *);
		px real_xml = (px)dlsym(RTLD_NEXT, "plist_to_xml");
		if( real_xml && options ) {
			real_xml(options, &xml, &xlen);
			if( xml ) {
				fprintf(stderr, "AQEMU: StartRestore options XML (%u bytes)\n", xlen);
				if( strstr(xml, "CreateFilesystemPartitions") )
					fprintf(stderr, "%s\n",
						strstr(xml, "CreateFilesystemPartitions"));
				else
					fprintf(stderr, "AQEMU: options XML has no CreateFilesystemPartitions\n");
				free(xml);
			}
		}
	}
	if( aqemu_rsr_hooked )
		return aqemu_call_real_rsr(client, options, version);
	if( ! real )
		real = (fn)dlsym(RTLD_NEXT, "restored_start_restore");
	if( ! real || real == restored_start_restore )
		return -1;
	return real(client, options, version);
}

restored_error_t restored_send(restored_client_t client, plist_t plist)
{
	typedef restored_error_t (*fn)(restored_client_t, plist_t);
	static fn real;
	static int nlog;
	if( nlog < 8 ) {
		nlog++;
		aqemu_log("AQEMU: restored_send ENTER #%d hooked=%d\n", nlog, aqemu_rsd_hooked);
	}
	if( plist ) {
		typedef plist_t (*get_fn)(plist_t, const char *);
		get_fn get = (get_fn)dlsym(RTLD_NEXT, "plist_dict_get_item");
		if( get ) {
			plist_t req = get(plist, "Request");
			plist_t opts = get(plist, "Options");
			if( ! opts )
				opts = get(plist, "RestoreOptions");
			if( opts || (req && nlog <= 2) )
				force_true(opts ? opts : plist);
		} else
			force_true(plist);
	}
	if( aqemu_rsd_hooked )
		return aqemu_call_real_rsd(client, plist);
	if( ! real )
		real = (fn)dlsym(RTLD_NEXT, "restored_send");
	if( ! real || real == restored_send )
		return -1;
	return real(client, plist);
}

/* idevicerestore's PLT does not call our plist/restored hooks (bind DEFAULT is
 * ours, but no hook logs). Restore plist goes to usbmuxd as write/send.
 * Interpose write/send paths; copy+patch restore plists on the wire.
 * Skip fd 1/2 (avoid fprintf recursion). */
static __thread int aqemu_in_io;

ssize_t write(int fd, const void *buf, size_t count)
{
	static ssize_t (*real)(int, const void *, size_t);
	void *copy;
	ssize_t r;
	if( ! real )
		real = (ssize_t (*)(int, const void *, size_t))dlsym(RTLD_NEXT, "write");
	if( ! real )
		return -1;
	if( aqemu_in_io || fd < 3 || ! buf_is_restore_wire(buf, count) )
		return real(fd, buf, count);
	aqemu_in_io = 1;
	fprintf(stderr, "AQEMU: write saw restore plist (%zu bytes)\n", count);
	copy = copy_patch(buf, count);
	r = real(fd, copy ? copy : buf, count);
	free(copy);
	aqemu_in_io = 0;
	return r;
}

ssize_t send(int sockfd, const void *buf, size_t len, int flags)
{
	static ssize_t (*real)(int, const void *, size_t, int);
	void *copy;
	ssize_t r;
	if( ! real )
		real = (ssize_t (*)(int, const void *, size_t, int))dlsym(RTLD_NEXT, "send");
	if( ! real )
		return -1;
	if( aqemu_in_io || sockfd < 3 || ! buf_is_restore_wire(buf, len) )
		return real(sockfd, buf, len, flags);
	aqemu_in_io = 1;
	fprintf(stderr, "AQEMU: send saw restore plist (%zu bytes)\n", len);
	copy = copy_patch(buf, len);
	r = real(sockfd, copy ? copy : buf, len, flags);
	free(copy);
	aqemu_in_io = 0;
	return r;
}

int usbmuxd_send(int sfd, const char *data, uint32_t len, uint32_t *sent)
{
	static int (*real)(int, const char *, uint32_t, uint32_t *);
	void *copy;
	int r;
	if( ! real )
		real = (int (*)(int, const char *, uint32_t, uint32_t *))
			dlsym(RTLD_NEXT, "usbmuxd_send");
	if( ! real )
		return -1;
	if( aqemu_in_io || ! buf_is_restore_wire(data, (size_t)len) )
		return real(sfd, data, len, sent);
	aqemu_in_io = 1;
	fprintf(stderr, "AQEMU: usbmuxd_send saw restore plist (%u bytes)\n", len);
	copy = copy_patch(data, (size_t)len);
	r = real(sfd, copy ? (const char *)copy : data, len, sent);
	free(copy);
	aqemu_in_io = 0;
	return r;
}

ssize_t writev(int fd, const struct iovec *iov, int iovcnt)
{
	static ssize_t (*real)(int, const struct iovec *, int);
	int i;
	if( ! real )
		real = (ssize_t (*)(int, const struct iovec *, int))dlsym(RTLD_NEXT, "writev");
	if( ! real )
		return -1;
	if( aqemu_in_io || fd < 3 || ! iov )
		return real(fd, iov, iovcnt);
	for( i = 0; i < iovcnt; i++ ) {
		if( ! buf_is_restore_wire(iov[i].iov_base, iov[i].iov_len) )
			continue;
		aqemu_in_io = 1;
		fprintf(stderr, "AQEMU: writev[%d] saw restore plist (%zu bytes)\n",
			i, (size_t)iov[i].iov_len);
		if( iov[i].iov_base )
			patch_payload(iov[i].iov_base, iov[i].iov_len);
		aqemu_in_io = 0;
		break;
	}
	return real(fd, iov, iovcnt);
}

ssize_t sendmsg(int sockfd, const struct msghdr *msg, int flags)
{
	static ssize_t (*real)(int, const struct msghdr *, int);
	size_t i;
	if( ! real )
		real = (ssize_t (*)(int, const struct msghdr *, int))dlsym(RTLD_NEXT, "sendmsg");
	if( ! real )
		return -1;
	if( aqemu_in_io || ! msg || ! msg->msg_iov )
		return real(sockfd, msg, flags);
	for( i = 0; i < msg->msg_iovlen; i++ ) {
		if( ! buf_is_restore_wire(msg->msg_iov[i].iov_base, msg->msg_iov[i].iov_len) )
			continue;
		aqemu_in_io = 1;
		fprintf(stderr, "AQEMU: sendmsg[%zu] saw restore plist (%zu bytes)\n",
			i, (size_t)msg->msg_iov[i].iov_len);
		if( msg->msg_iov[i].iov_base )
			patch_payload(msg->msg_iov[i].iov_base, msg->msg_iov[i].iov_len);
		aqemu_in_io = 0;
		break;
	}
	return real(sockfd, msg, flags);
}

static void *dyn_ptr(ElfW(Addr) base, ElfW(Addr) p)
{
	if( p == 0 )
		return NULL;
	if( p < base )
		return (void *)(base + p);
	return (void *)p;
}

static int got_make_writable(void *slot)
{
	long ps = sysconf(_SC_PAGESIZE);
	uintptr_t pg;
	if( ps <= 0 )
		ps = 4096;
	pg = (uintptr_t)slot & ~((uintptr_t)ps - 1);
	return mprotect((void *)pg, (size_t)ps, PROT_READ | PROT_WRITE);
}

static void *got_want(const char *nm)
{
	if( strcmp(nm, "restored_start_restore") == 0 )
		return (void *)restored_start_restore;
	if( strcmp(nm, "restored_send") == 0 )
		return (void *)restored_send;
	if( strcmp(nm, "plist_dict_set_item") == 0 )
		return (void *)plist_dict_set_item;
	if( strcmp(nm, "plist_to_bin") == 0 )
		return (void *)plist_to_bin;
	if( strcmp(nm, "plist_to_xml") == 0 )
		return (void *)plist_to_xml;
	if( strcmp(nm, "SSL_write") == 0 )
		return (void *)SSL_write;
	if( strcmp(nm, "SSL_write_ex") == 0 )
		return (void *)SSL_write_ex;
	if( strcmp(nm, "usbmuxd_send") == 0 )
		return (void *)usbmuxd_send;
	if( strcmp(nm, "idevice_connection_send") == 0 )
		return (void *)idevice_connection_send;
	return NULL;
}

static void rewire_rela_table(ElfW(Addr) base, ElfW(Rela) *rela, size_t n,
	ElfW(Sym) *symtab, const char *strtab, const char *oname)
{
	size_t k;
	const char *tag = oname[0] ? oname : "exe";
	for( k = 0; k < n; k++ ) {
		int type = ELF64_R_TYPE(rela[k].r_info);
		int symn = (int)ELF64_R_SYM(rela[k].r_info);
		const char *nm;
		void **slot;
		void *want;
		if( type != R_X86_64_JUMP_SLOT && type != R_X86_64_GLOB_DAT )
			continue;
		nm = strtab + symtab[symn].st_name;
		want = got_want(nm);
		if( ! want )
			continue;
		slot = (void **)(base + rela[k].r_offset);
		if( got_make_writable(slot) != 0 )
			slot = (void **)(uintptr_t)rela[k].r_offset;
		if( got_make_writable(slot) != 0 ) {
			fprintf(stderr, "AQEMU: GOT mprotect fail %s %s slot=%p\n",
				tag, nm, (void *)slot);
			continue;
		}
		if( *slot == want )
			continue;
		fprintf(stderr, "AQEMU: GOT %s %s %p -> %p\n", tag, nm, *slot, want);
		*slot = want;
	}
}

static int got_rewire_cb(struct dl_phdr_info *info, size_t size, void *data)
{
	const ElfW(Phdr) *ph;
	const ElfW(Dyn) *dyn = NULL;
	ElfW(Addr) base = info->dlpi_addr;
	ElfW(Sym) *symtab = NULL;
	const char *strtab = NULL;
	ElfW(Rela) *jmp = NULL;
	ElfW(Rela) *rela = NULL;
	size_t jmpsz = 0, relasz = 0;
	const char *oname = info->dlpi_name ? info->dlpi_name : "";
	int i;
	(void)size;
	(void)data;
	if( strstr(oname, "libimobiledevice-glue") )
		return 0;
	if( oname[0] != '\0' &&
	    ! strstr(oname, "idevicerestore") &&
	    ! strstr(oname, "libimobiledevice-1") &&
	    ! strstr(oname, "libusbmuxd") )
		return 0;
	for( i = 0; i < info->dlpi_phnum; i++ ) {
		ph = &info->dlpi_phdr[i];
		if( ph->p_type == PT_DYNAMIC ) {
			dyn = (const ElfW(Dyn) *)(base + ph->p_vaddr);
			break;
		}
	}
	if( ! dyn )
		return 0;
	for( ; dyn->d_tag != DT_NULL; dyn++ ) {
		if( dyn->d_tag == DT_SYMTAB )
			symtab = dyn_ptr(base, dyn->d_un.d_ptr);
		else if( dyn->d_tag == DT_STRTAB )
			strtab = dyn_ptr(base, dyn->d_un.d_ptr);
		else if( dyn->d_tag == DT_JMPREL )
			jmp = dyn_ptr(base, dyn->d_un.d_ptr);
		else if( dyn->d_tag == DT_PLTRELSZ )
			jmpsz = (size_t)dyn->d_un.d_val;
		else if( dyn->d_tag == DT_RELA )
			rela = dyn_ptr(base, dyn->d_un.d_ptr);
		else if( dyn->d_tag == DT_RELASZ )
			relasz = (size_t)dyn->d_un.d_val;
	}
	fprintf(stderr, "AQEMU: phdr '%s' base=%p pltrel=%zu rela=%zu\n",
		oname[0] ? oname : "exe", (void *)(uintptr_t)base, jmpsz, relasz);
	if( ! symtab || ! strtab ) {
		fprintf(stderr, "AQEMU: no SYMTAB in %s\n", oname[0] ? oname : "exe");
		return 0;
	}
	if( jmp && jmpsz >= sizeof(ElfW(Rela)) )
		rewire_rela_table(base, jmp, jmpsz / sizeof(ElfW(Rela)),
			symtab, strtab, oname);
	if( rela && relasz >= sizeof(ElfW(Rela)) )
		rewire_rela_table(base, rela, relasz / sizeof(ElfW(Rela)),
			symtab, strtab, oname);
	if( ( ! jmp || jmpsz < sizeof(ElfW(Rela)) ) &&
	    ( ! rela || relasz < sizeof(ElfW(Rela)) ) )
		fprintf(stderr, "AQEMU: no JMPREL/RELA in %s base=%p\n",
			oname[0] ? oname : "exe", (void *)(uintptr_t)base);
	return 0;
}

static void aqemu_got_rewire(void)
{
	dl_iterate_phdr(got_rewire_cb, NULL);
}

struct aqemu_bias {
	unsigned long bias;
	int found;
};

static int aqemu_bias_cb(struct dl_phdr_info *info, size_t size, void *data)
{
	struct aqemu_bias *o = data;
	const char *n = info->dlpi_name ? info->dlpi_name : "";
	(void)size;
	if( n[0] == '\0' ) {
		o->bias = (unsigned long)info->dlpi_addr;
		o->found = 1;
		return 1;
	}
	if( strstr(n, "idevicerestore") && ! strstr(n, ".so") ) {
		o->bias = (unsigned long)info->dlpi_addr;
		o->found = 1;
		return 1;
	}
	return 0;
}

static unsigned long aqemu_exe_bias(void)
{
	struct aqemu_bias o;
	o.bias = 0;
	o.found = 0;
	dl_iterate_phdr(aqemu_bias_cb, &o);
	if( o.found )
		return o.bias;
	{
		FILE *f = fopen("/proc/self/maps", "r");
		char line[640], path[400];
		unsigned long start = 0;
		if( ! f )
			return 0;
		while( fgets(line, (int)sizeof(line), f) ) {
			path[0] = 0;
			if( sscanf(line, "%lx-%*x %*s %*s %*s %*s %399s", &start, path) < 1 )
				continue;
			if( strstr(path, "idevicerestore") && ! strstr(path, ".so") ) {
				fclose(f);
				return start;
			}
		}
		fclose(f);
	}
	return 0;
}

static void aqemu_patch_one_rela(unsigned long bias, Elf64_Rela *rela, size_t nrela,
	Elf64_Sym *sym, const char *str, const char *tag)
{
	size_t i;
	for( i = 0; i < nrela; i++ ) {
		unsigned int type = ELF64_R_TYPE(rela[i].r_info);
		unsigned int si = (unsigned int)ELF64_R_SYM(rela[i].r_info);
		const char *nm;
		void **slot;
		void *want;
		if( type != R_X86_64_JUMP_SLOT && type != R_X86_64_GLOB_DAT )
			continue;
		nm = str + sym[si].st_name;
		want = got_want(nm);
		if( ! want )
			continue;
		slot = (void **)( bias + (unsigned long)rela[i].r_offset );
		if( got_make_writable(slot) != 0 ) {
			fprintf(stderr, "AQEMU: exe GOT mprotect fail %s %s slot=%p\n",
				tag, nm, (void *)slot);
			continue;
		}
		fprintf(stderr, "AQEMU: exe GOT (%s) %s %p -> %p\n", tag, nm, *slot, want);
		*slot = want;
	}
}

static void aqemu_patch_exe_got(void)
{
	int fd;
	unsigned char *map;
	off_t sz;
	Elf64_Ehdr *eh;
	Elf64_Shdr *sh;
	Elf64_Shdr *dynsym = NULL, *dynstr = NULL, *relaplt = NULL, *reladyn = NULL;
	Elf64_Sym *sym;
	const char *str;
	size_t i, shnum;
	unsigned long bias;

	bias = aqemu_exe_bias();
	fprintf(stderr, "AQEMU: exe load bias=0x%lx\n", bias);
	fd = open("/proc/self/exe", O_RDONLY);
	if( fd < 0 ) {
		fprintf(stderr, "AQEMU: open /proc/self/exe failed errno=%d\n", errno);
		return;
	}
	sz = lseek(fd, 0, SEEK_END);
	if( sz < 64 ) {
		close(fd);
		return;
	}
	map = mmap(NULL, (size_t)sz, PROT_READ, MAP_PRIVATE, fd, 0);
	close(fd);
	if( map == MAP_FAILED ) {
		fprintf(stderr, "AQEMU: mmap exe failed errno=%d\n", errno);
		return;
	}
	eh = (Elf64_Ehdr *)map;
	if( memcmp(eh->e_ident, ELFMAG, SELFMAG) != 0 || eh->e_ident[EI_CLASS] != ELFCLASS64 ) {
		munmap(map, (size_t)sz);
		return;
	}
	sh = (Elf64_Shdr *)( map + eh->e_shoff );
	shnum = eh->e_shnum;
	{
		const char *shstr = (const char *)( map + sh[eh->e_shstrndx].sh_offset );
		for( i = 0; i < shnum; i++ ) {
			const char *nm = shstr + sh[i].sh_name;
			if( sh[i].sh_type == SHT_DYNSYM )
				dynsym = &sh[i];
			else if( strcmp(nm, ".rela.plt") == 0 )
				relaplt = &sh[i];
			else if( strcmp(nm, ".rela.dyn") == 0 )
				reladyn = &sh[i];
		}
	}
	if( dynsym && dynsym->sh_link < shnum )
		dynstr = &sh[dynsym->sh_link];
	if( ! dynsym || ! dynstr || ( eh->e_type == ET_DYN && bias == 0 ) ) {
		fprintf(stderr, "AQEMU: exe GOT parse type=%d dynsym=%p bias=0x%lx plt=%p dyn=%p\n",
			(int)eh->e_type, (void *)dynsym, bias, (void *)relaplt, (void *)reladyn);
		munmap(map, (size_t)sz);
		return;
	}
	sym = (Elf64_Sym *)( map + dynsym->sh_offset );
	str = (const char *)( map + dynstr->sh_offset );
	if( relaplt ) {
		fprintf(stderr, "AQEMU: exe .rela.plt entries=%zu\n",
			(size_t)( relaplt->sh_size / sizeof(Elf64_Rela) ));
		aqemu_patch_one_rela(bias, (Elf64_Rela *)( map + relaplt->sh_offset ),
			relaplt->sh_size / sizeof(Elf64_Rela), sym, str, ".rela.plt");
	}
	if( reladyn ) {
		fprintf(stderr, "AQEMU: exe .rela.dyn entries=%zu\n",
			(size_t)( reladyn->sh_size / sizeof(Elf64_Rela) ));
		aqemu_patch_one_rela(bias, (Elf64_Rela *)( map + reladyn->sh_offset ),
			reladyn->sh_size / sizeof(Elf64_Rela), sym, str, ".rela.dyn");
	}
	if( ! relaplt && ! reladyn )
		fprintf(stderr, "AQEMU: exe has no .rela.plt or .rela.dyn\n");
	munmap(map, (size_t)sz);
}
