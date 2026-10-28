/* One-shot registrar for apps copied into /Applications.
 * Stock iOS does not notice a new bundle there, and this image has no uicache.
 * Launchd starts this after boot. It waits until the data volume is mounted,
 * registers each path in /usr/libexec/aqemu-register.list, then restarts
 * SpringBoard once. A marker on the data volume stops later boots from
 * restarting SpringBoard again.
 *
 * Rebuild with extras/Inferno/build-aqemu-register.py (WSL clang, iOS arm64).
 */
typedef struct objc_object *id;
typedef struct objc_selector *SEL;
typedef unsigned int uint32_t;

void *dlopen(const char *path, int mode);
void *dlsym(void *handle, const char *symbol);
unsigned int sleep(unsigned int seconds);
int notify_post(const char *name);
int proc_listpids(uint32_t type, uint32_t typeinfo, void *buffer, int buffersize);
int proc_pidpath(int pid, void *buffer, uint32_t buffersize);
int kill(int pid, int sig);
int access(const char *path, int mode);
int open(const char *path, int flags, int mode);
long read(int fd, void *buf, unsigned long n);
long write(int fd, const void *buf, unsigned long n);
int close(int fd);
int fsync(int fd);

static void *memset(void *s, int c, unsigned long n) {
    unsigned char *p = (unsigned char *)s;
    while (n--)
        *p++ = (unsigned char)c;
    return s;
}

static void *memcpy(void *d, const void *s, unsigned long n) {
    unsigned char *dd = (unsigned char *)d;
    const unsigned char *ss = (const unsigned char *)s;
    while (n--)
        *dd++ = *ss++;
    return d;
}

#define RTLD_NOW 0x2
#define RTLD_GLOBAL 0x8
#define O_WRONLY 0x1
#define O_RDWR 0x2
#define O_CREAT 0x0200
#define O_TRUNC 0x0400
#define O_APPEND 0x0008
#define PROC_ALL_PIDS 1

static int g_log = -1;
static void *g_send;
static SEL (*g_sel)(const char *);
static id (*g_class)(const char *);

static unsigned long slen(const char *s) {
    unsigned long n = 0;
    while (s[n])
        n++;
    return n;
}

static void logstr(const char *s) {
    if (g_log >= 0)
        write(g_log, s, slen(s));
}

static void logline(const char *s) {
    logstr(s);
    logstr("\n");
}

static int contains(const char *s, const char *part) {
    unsigned long n = slen(part);
    unsigned long i = 0;
    if (n == 0)
        return 1;
    for (; s[i]; i++) {
        unsigned long j = 0;
        while (j < n && s[i + j] == part[j])
            j++;
        if (j == n)
            return 1;
    }
    return 0;
}

static int same_bytes(const char *a, unsigned long na, const char *b, unsigned long nb) {
    unsigned long i;
    if (na != nb)
        return 0;
    for (i = 0; i < na; i++) {
        if (a[i] != b[i])
            return 0;
    }
    return 1;
}

static id send0(id self, const char *sel) {
    return ((id (*)(id, SEL))g_send)(self, g_sel(sel));
}

static id send_obj(id self, const char *sel, id a) {
    return ((id (*)(id, SEL, id))g_send)(self, g_sel(sel), a);
}

static id send_cstr(id self, const char *sel, const char *a) {
    return ((id (*)(id, SEL, const char *))g_send)(self, g_sel(sel), a);
}

static void send_obj2(id self, const char *sel, id a, id b) {
    ((void (*)(id, SEL, id, id))g_send)(self, g_sel(sel), a, b);
}

static signed char send_bool(id self, const char *sel, id a) {
    return ((signed char (*)(id, SEL, id))g_send)(self, g_sel(sel), a);
}

static id nsstr(const char *s) {
    return send_cstr((id)g_class("NSString"), "stringWithUTF8String:", s);
}

static int load_objc(void) {
    void *objc;
    void *foundation;
    void *cs;
    objc = dlopen("/usr/lib/libobjc.A.dylib", RTLD_NOW | RTLD_GLOBAL);
    if (!objc) {
        logline("dlopen libobjc failed");
        return 0;
    }
    g_class = (id (*)(const char *))dlsym(objc, "objc_getClass");
    g_sel = (SEL (*)(const char *))dlsym(objc, "sel_registerName");
    g_send = dlsym(objc, "objc_msgSend");
    if (!g_class || !g_sel || !g_send) {
        logline("objc symbols missing");
        return 0;
    }
    foundation = dlopen("/System/Library/Frameworks/Foundation.framework/Foundation", RTLD_NOW | RTLD_GLOBAL);
    if (!foundation) {
        logline("dlopen Foundation failed");
        return 0;
    }
    cs = dlopen("/System/Library/Frameworks/MobileCoreServices.framework/MobileCoreServices", RTLD_NOW | RTLD_GLOBAL);
    if (!cs)
        cs = dlopen("/System/Library/Frameworks/CoreServices.framework/CoreServices", RTLD_NOW | RTLD_GLOBAL);
    if (!cs) {
        logline("dlopen CoreServices failed");
        return 0;
    }
    return 1;
}

static int register_one(const char *app_path) {
    id pool;
    id info;
    id dict;
    id ws_cls;
    id ws;
    id plist;
    id app;
    signed char ok_dict;
    signed char ok_url;
    char plist_path[1024];
    unsigned long n;
    unsigned long i;

    n = slen(app_path);
    if (n == 0 || n + 12 >= sizeof plist_path)
        return 0;
    for (i = 0; i < n; i++)
        plist_path[i] = app_path[i];
    plist_path[n] = 0;
    /* drop a trailing slash */
    if (n > 1 && plist_path[n - 1] == '/') {
        plist_path[n - 1] = 0;
        n--;
    }
    {
        static const char suffix[] = "/Info.plist";
        for (i = 0; suffix[i]; i++)
            plist_path[n + i] = suffix[i];
        plist_path[n + i] = 0;
    }

    logstr("register ");
    logline(app_path);
    if (access(plist_path, 0) != 0) {
        logline("Info.plist missing");
        return 0;
    }

    pool = send0((id)g_class("NSAutoreleasePool"), "new");
    app = nsstr(app_path);
    plist = nsstr(plist_path);
    info = send_obj((id)g_class("NSDictionary"), "dictionaryWithContentsOfFile:", plist);
    if (!info) {
        logline("could not read Info.plist");
        if (pool)
            send0(pool, "drain");
        return 0;
    }
    dict = send0(info, "mutableCopy");
    if (!dict) {
        logline("mutableCopy failed");
        if (pool)
            send0(pool, "drain");
        return 0;
    }
    send_obj2(dict, "setObject:forKey:", app, nsstr("Path"));
    send_obj2(dict, "setObject:forKey:", app, nsstr("ApplicationPath"));
    send_obj2(dict, "setObject:forKey:", nsstr("System"), nsstr("ApplicationType"));

    ws_cls = (id)g_class("LSApplicationWorkspace");
    if (!ws_cls) {
        logline("LSApplicationWorkspace missing");
        if (pool)
            send0(pool, "drain");
        return 0;
    }
    ws = send0(ws_cls, "defaultWorkspace");
    if (!ws) {
        logline("defaultWorkspace missing");
        if (pool)
            send0(pool, "drain");
        return 0;
    }
    ok_dict = send_bool(ws, "registerApplicationDictionary:", dict);
    logline(ok_dict ? "registerApplicationDictionary yes" : "registerApplicationDictionary no");

    ok_url = 0;
    if (!ok_dict) {
        id url = send_obj((id)g_class("NSURL"), "fileURLWithPath:", app);
        if (url) {
            ok_url = send_bool(ws, "registerApplication:", url);
            logline(ok_url ? "registerApplication yes" : "registerApplication no");
        }
    }
    if (pool)
        send0(pool, "drain");
    return ok_dict || ok_url;
}

static void restart_springboard(void) {
    static int pids[1024];
    static char path[1024];
    int bytes;
    int n;
    int i;

    notify_post("com.apple.mobile.application_installed");
    bytes = proc_listpids(PROC_ALL_PIDS, 0, pids, (int)sizeof pids);
    if (bytes < 4)
        return;
    n = bytes / 4;
    if (n > 1024)
        n = 1024;
    for (i = 0; i < n; i++) {
        int len;
        if (pids[i] <= 1)
            continue;
        memset(path, 0, sizeof path);
        len = proc_pidpath(pids[i], path, (uint32_t)sizeof path);
        if (len <= 0)
            continue;
        if (contains(path, "/SpringBoard.app/SpringBoard")) {
            logstr("restart ");
            logline(path);
            kill(pids[i], 9);
        }
    }
}

static int read_file(const char *path, char *buf, unsigned long cap, unsigned long *out_n) {
    int fd;
    long n;
    unsigned long total = 0;
    fd = open(path, 0, 0);
    if (fd < 0)
        return 0;
    while (total + 1 < cap) {
        n = read(fd, buf + total, cap - 1 - total);
        if (n <= 0)
            break;
        total += (unsigned long)n;
    }
    close(fd);
    buf[total] = 0;
    *out_n = total;
    return 1;
}

static void write_marker(const char *buf, unsigned long n) {
    int fd = open("/var/mobile/Library/aqemu-register.done", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        logline("could not write marker");
        return;
    }
    write(fd, buf, n);
    fsync(fd);
    close(fd);
}

int main(void) {
    static char listbuf[4096];
    static char donebuf[4096];
    static char line[1024];
    unsigned long list_n = 0;
    unsigned long done_n = 0;
    unsigned long i;
    int waited;
    int any = 0;
    const char *fallback = "/Applications/UTM SE.app\n";

    g_log = open("/usr/libexec/aqemu-register.log", O_WRONLY | O_CREAT | O_APPEND, 0644);
    logline("aqemu-register start");

    for (waited = 0; waited < 90; waited++) {
        if (access("/var/mobile/Library/SpringBoard", 0) == 0)
            break;
        sleep(2);
    }
    if (access("/var/mobile/Library/SpringBoard", 0) != 0) {
        logline("data volume not ready");
        if (g_log >= 0)
            close(g_log);
        return 0;
    }
    /* Let SpringBoard finish its first icon build, then replace it. */
    sleep(5);

    if (!read_file("/usr/libexec/aqemu-register.list", listbuf, sizeof listbuf, &list_n) || list_n == 0) {
        unsigned long n = slen(fallback);
        memcpy(listbuf, fallback, n);
        listbuf[n] = 0;
        list_n = n;
        logline("using default UTM SE path");
    }
    if (read_file("/var/mobile/Library/aqemu-register.done", donebuf, sizeof donebuf, &done_n) &&
        same_bytes(listbuf, list_n, donebuf, done_n)) {
        logline("already registered");
        if (g_log >= 0)
            close(g_log);
        return 0;
    }
    if (!load_objc()) {
        if (g_log >= 0)
            close(g_log);
        return 0;
    }

    i = 0;
    while (i < list_n) {
        unsigned long start = i;
        unsigned long len;
        while (i < list_n && listbuf[i] != '\n')
            i++;
        len = i - start;
        if (len > 0 && listbuf[start + len - 1] == '\r')
            len--;
        if (len > 0 && len < sizeof line) {
            unsigned long j;
            for (j = 0; j < len; j++)
                line[j] = listbuf[start + j];
            line[len] = 0;
            if (register_one(line))
                any = 1;
        }
        if (i < list_n && listbuf[i] == '\n')
            i++;
    }

    if (any) {
        write_marker(listbuf, list_n);
        sleep(1);
        restart_springboard();
        logline("done");
    } else {
        logline("nothing registered");
    }
    if (g_log >= 0)
        close(g_log);
    return 0;
}
