/* Minimal implementations for compatibility layer. Not thread-safe.
 * Keeps a small static heap and provides basic string and printf support.
 */

#include "compat.h"
#include <unistd.h>
#include <stdint.h>

/* Simple heap: bump allocator with free list */
typedef struct block {
    size_t size;
    struct block *next;
    int free;
} block_t;

#define HEAP_SIZE (256 * 1024)
static unsigned char heap[HEAP_SIZE];
static int heap_init = 0;
static block_t *free_list = NULL;
static void *heap_start = (void*)0;

static void heap_setup(void)
{
    heap_start = heap;
    free_list = (block_t*)heap_start;
    free_list->size = HEAP_SIZE - sizeof(block_t);
    free_list->next = NULL;
    free_list->free = 1;
    heap_init = 1;
}

static void split_block(block_t *b, size_t size)
{
    if (b->size >= size + sizeof(block_t) + 8) {
        block_t *nb = (block_t*)((unsigned char*)b + sizeof(block_t) + size);
        nb->size = b->size - size - sizeof(block_t);
        nb->next = b->next;
        nb->free = 1;
        b->size = size;
        b->next = nb;
    }
}

void *nv_malloc(size_t size)
{
    if (!heap_init) heap_setup();
    block_t *cur = free_list;
    while (cur) {
        if (cur->free && cur->size >= size) {
            split_block(cur, size);
            cur->free = 0;
            return (unsigned char*)cur + sizeof(block_t);
        }
        cur = cur->next;
    }
    return NULL; /* out of memory */
}

void nv_free(void *ptr)
{
    if (!ptr) return;
    block_t *b = (block_t*)((unsigned char*)ptr - sizeof(block_t));
    b->free = 1;
    /* coalesce simple */
    block_t *cur = free_list;
    while (cur) {
        if (cur->free && cur->next && cur->next->free) {
            cur->size += sizeof(block_t) + cur->next->size;
            cur->next = cur->next->next;
            continue;
        }
        cur = cur->next;
    }
}

void *nv_realloc(void *ptr, size_t size)
{
    if (!ptr) return nv_malloc(size);
    if (size == 0) { nv_free(ptr); return NULL; }
    block_t *b = (block_t*)((unsigned char*)ptr - sizeof(block_t));
    if (b->size >= size) return ptr;
    void *newp = nv_malloc(size);
    if (!newp) return NULL;
    size_t copy = b->size < size ? b->size : size;
    nv_memcpy(newp, ptr, copy);
    nv_free(ptr);
    return newp;
}

void *nv_calloc(size_t nmemb, size_t size)
{
    size_t total = nmemb * size;
    void *p = nv_malloc(total);
    if (p) nv_memset(p, 0, total);
    return p;
}

void *nv_memcpy(void *dest, const void *src, size_t n)
{
    unsigned char *d = dest;
    const unsigned char *s = src;
    while (n--) *d++ = *s++;
    return dest;
}

void *nv_memset(void *s, int c, size_t n)
{
    unsigned char *p = s;
    while (n--) *p++ = (unsigned char)c;
    return s;
}

int nv_memcmp(const void *s1, const void *s2, size_t n)
{
    const unsigned char *a = s1, *b = s2;
    for (size_t i = 0; i < n; ++i) if (a[i] != b[i]) return (int)a[i] - (int)b[i];
    return 0;
}

size_t nv_strlen(const char *s)
{
    const char *p = s;
    while (*p) ++p;
    return (size_t)(p - s);
}

int nv_strcmp(const char *s1, const char *s2)
{
    while (*s1 && (*s1 == *s2)) { s1++; s2++; }
    return (unsigned char)*s1 - (unsigned char)*s2;
}

int nv_strncmp(const char *s1, const char *s2, size_t n)
{
    for (size_t i = 0; i < n; ++i) {
        if (s1[i] != s2[i] || s1[i] == '\0' || s2[i] == '\0')
            return (unsigned char)s1[i] - (unsigned char)s2[i];
    }
    return 0;
}

char *nv_strcpy(char *dest, const char *src)
{
    char *d = dest;
    while ((*d++ = *src++));
    return dest;
}

char *nv_strncpy(char *dest, const char *src, size_t n)
{
    size_t i;
    for (i = 0; i < n && src[i]; ++i) dest[i] = src[i];
    for (; i < n; ++i) dest[i] = '\0';
    return dest;
}

char *nv_strdup(const char *s)
{
    size_t len = nv_strlen(s) + 1;
    char *p = nv_malloc(len);
    if (!p) return NULL;
    nv_memcpy(p, s, len);
    return p;
}

char *nv_strstr(const char *haystack, const char *needle)
{
    if (!*needle) return (char*)haystack;
    for (; *haystack; ++haystack) {
        const char *h = haystack, *n = needle;
        while (*h && *n && *h == *n) { ++h; ++n; }
        if (!*n) return (char*)haystack;
    }
    return NULL;
}

/* Minimal printf implementation: supports %s, %d, %u, %x, %c */
/* forward declarations for syscall-backed helpers used below */
static int nv_sys_write(int fd, const void *buf, unsigned int count);
static int nv_sys_read(int fd, void *buf, unsigned int count);
static int nv_sys_open(const char *path, int flags, int mode);
static int nv_sys_close(int fd);

static void put_char(char c)
{
    nv_sys_write(1, &c, 1);
}

static void put_str(const char *s)
{
    nv_sys_write(1, s, nv_strlen(s));
}

static void put_uint(unsigned int v, int base, int is_upper)
{
    char buf[32];
    int i = 0;
    if (v == 0) { put_char('0'); return; }
    while (v) {
        int d = v % base;
        buf[i++] = (d < 10) ? ('0' + d) : ((is_upper ? 'A' : 'a') + (d - 10));
        v /= base;
    }
    while (i--) put_char(buf[i]);
}

int nv_vprintf(const char *fmt, va_list ap)
{
    const char *p = fmt;
    while (*p) {
        if (*p != '%') { put_char(*p++); continue; }
        ++p;
        if (*p == '%') { put_char('%'); ++p; continue; }
        switch (*p) {
            case 's': { char *s = va_arg(ap, char*); if (!s) s = "(null)"; put_str(s); break; }
            case 'd': { int v = va_arg(ap, int); if (v < 0) { put_char('-'); put_uint((unsigned int)(-v),10,0); } else put_uint((unsigned int)v,10,0); break; }
            case 'u': { unsigned int v = va_arg(ap, unsigned int); put_uint(v,10,0); break; }
            case 'x': { unsigned int v = va_arg(ap, unsigned int); put_uint(v,16,0); break; }
            case 'X': { unsigned int v = va_arg(ap, unsigned int); put_uint(v,16,1); break; }
            case 'c': { char c = (char)va_arg(ap, int); put_char(c); break; }
            default: put_char('?'); break;
        }
        ++p;
    }
    return 0;
}

int nv_printf(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int r = nv_vprintf(fmt, ap);
    va_end(ap);
    return r;
}

int nv_fprintf(int fd, const char *fmt, ...)
{
    /* For simplicity, ignore fd and write to stdout */
    (void)fd;
    va_list ap;
    va_start(ap, fmt);
    int r = nv_vprintf(fmt, ap);
    va_end(ap);
    return r;
}

int nvram_faker_strcmp(const char *a, const char *b)
{
    return nv_strcmp(a, b);
}

int nv_atoi(const char *s)
{
    int sign = 1;
    int val = 0;
    while (*s == ' ' || *s == '\t' || *s == '\n') ++s;
    if (*s == '-') { sign = -1; ++s; }
    else if (*s == '+') ++s;
    while (*s >= '0' && *s <= '9') { val = val * 10 + (*s - '0'); ++s; }
    return sign * val;
}

unsigned int nv_sleep(unsigned int seconds)
{
    /* Implement sleep without libc: for ARM use busy loop, otherwise use usleep */
#ifdef __arm__
    volatile unsigned long i;
    /* crude busy-wait calibrated for ~1 second on typical embedded CPU; may be noisy */
    for (i = 0; i < (unsigned long)seconds * 10000000UL; ++i) { __asm__ __volatile__("nop"); }
    return 0;
#else
    unsigned int usec = seconds * 1000000U;
    usleep(usec);
    return 0;
#endif
}

/* --- syscall wrappers and minimal FILE-like API --- */

#ifdef __arm__
static inline long sys_call3(long n, long a, long b, long c)
{
    long ret;
    register long r0 __asm__("r0") = a;
    register long r1 __asm__("r1") = b;
    register long r2 __asm__("r2") = c;
    register long r7 __asm__("r7") = n;
    __asm__ __volatile__(
        "svc 0\n"
        : "=r" (r0)
        : "r" (r0), "r" (r1), "r" (r2), "r" (r7)
        : "memory");
    ret = r0;
    return ret;
}

static inline long sys_call1(long n, long a)
{
    long ret;
    register long r0 __asm__("r0") = a;
    register long r7 __asm__("r7") = n;
    __asm__ __volatile__(
        "svc 0\n"
        : "=r" (r0)
        : "r" (r0), "r" (r7)
        : "memory");
    ret = r0;
    return ret;
}

#define SYS_read 3
#define SYS_write 4
#define SYS_open 5
#define SYS_close 6
#define SYS_lseek 19
#define SYS_exit 1

static int nv_sys_open(const char *path, int flags, int mode)
{
    return (int)sys_call3(SYS_open, (long)path, flags, mode);
}

static int nv_sys_read(int fd, void *buf, unsigned int count)
{
    return (int)sys_call3(SYS_read, fd, (long)buf, count);
}

static int nv_sys_write(int fd, const void *buf, unsigned int count)
{
    return (int)sys_call3(SYS_write, fd, (long)buf, count);
}

static int nv_sys_close(int fd)
{
    return (int)sys_call1(SYS_close, fd);
}

static long nv_sys_lseek(int fd, long offset, int whence)
{
    /* lseek uses syscall number with different args; use sys_call3 */
    return sys_call3(SYS_lseek, fd, offset, whence);
}

static void nv_sys_exit(int status) __attribute__((noreturn));
static void nv_sys_exit(int status)
{
    sys_call1(SYS_exit, status);
    for(;;) { __asm__ __volatile__("nop"); }
}

#else /* non-arm: fallback to libc syscall() */
#include <sys/syscall.h>
#include <fcntl.h>

static int nv_sys_open(const char *path, int flags, int mode)
{
    return syscall(SYS_open, path, flags, mode);
}

static int nv_sys_read(int fd, void *buf, unsigned int count)
{
    return syscall(SYS_read, fd, buf, count);
}

static int nv_sys_write(int fd, const void *buf, unsigned int count)
{
    return syscall(SYS_write, fd, buf, count);
}

static int nv_sys_close(int fd)
{
    return syscall(SYS_close, fd);
}

static long nv_sys_lseek(int fd, long offset, int whence)
{
    return syscall(SYS_lseek, fd, offset, whence);
}

static void nv_sys_exit(int status) __attribute__((noreturn));
static void nv_sys_exit(int status)
{
    syscall(SYS_exit, status);
    __builtin_unreachable();
}

#endif

struct nv_FILE { int fd; };

FILE *nv_fopen(const char *path, const char *mode)
{
    (void)mode;
    int fd = nv_sys_open(path, /*O_RDONLY*/ 0, 0);
    if (fd < 0) return NULL;
    nv_FILE *f = nv_malloc(sizeof(nv_FILE));
    if (!f) { nv_sys_close(fd); return NULL; }
    f->fd = fd;
    return (FILE*)f;
}

char *nv_fgets(char *s, int size, FILE *stream)
{
    if (!s || size <= 0 || !stream) return NULL;
    nv_FILE *f = (nv_FILE*)stream;
    int i = 0;
    while (i < size - 1) {
        char c;
        int r = nv_sys_read(f->fd, &c, 1);
        if (r <= 0) break;
        s[i++] = c;
        if (c == '\n') break;
    }
    if (i == 0) return NULL;
    s[i] = '\0';
    return s;
}

int nv_fclose(FILE *stream)
{
    if (!stream) return -1;
    nv_FILE *f = (nv_FILE*)stream;
    int r = nv_sys_close(f->fd);
    nv_free(f);
    return r;
}

void nv_exit(int status)
{
    nv_sys_exit(status);
}

/* Provide minimal raise() stub to avoid linking libc's raise */
int raise(int sig)
{
    (void)sig;
    return 0;
}

/* Provide symbol-compatible wrappers so the linker does not pull these
 * from glibc. These are minimal and intentionally simple.
 */

/* undef the macro from compat.h so we can define the real symbol here */
#undef memset
void *memset(void *s, int c, size_t n)
{
    return nv_memset(s, c, n);
}

void __cxa_finalize(void *d)
{
    /* noop: suppress reference to libc's __cxa_finalize */
    (void)d;
}

const unsigned short *__ctype_b_loc(void)
{
    static const unsigned short table[1] = {0};
    return table;
}

