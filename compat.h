/* Minimal compatibility layer to replace common libc functions
 * so the codebase can be built against firmware-like environments.
 */
#ifndef COMPAT_H
#define COMPAT_H

#include <stddef.h>
#include <stdarg.h>

/* Allocation */
void *nv_malloc(size_t size);
void nv_free(void *ptr);
void *nv_realloc(void *ptr, size_t size);
void *nv_calloc(size_t nmemb, size_t size);

/* Memory */
void *nv_memcpy(void *dest, const void *src, size_t n);
void *nv_memset(void *s, int c, size_t n);
int nv_memcmp(const void *s1, const void *s2, size_t n);

/* Strings */
size_t nv_strlen(const char *s);
int nv_strcmp(const char *s1, const char *s2);
int nv_strncmp(const char *s1, const char *s2, size_t n);
char *nv_strcpy(char *dest, const char *src);
char *nv_strncpy(char *dest, const char *src, size_t n);
char *nv_strdup(const char *s);
char *nv_strstr(const char *haystack, const char *needle);

/* Small helpers */
int nvram_faker_strcmp(const char *a, const char *b);
int nv_atoi(const char *s);
unsigned int nv_sleep(unsigned int seconds);

/* I/O */
int nv_printf(const char *fmt, ...);
int nv_fprintf(int fd, const char *fmt, ...);

/* Minimal FILE-like API using syscalls (use FILE* for compatibility) */
#include <stdio.h>
typedef struct nv_FILE nv_FILE;
FILE *nv_fopen(const char *path, const char *mode);
char *nv_fgets(char *s, int size, FILE *stream);
int nv_fclose(FILE *stream);

/* exit replacement */
void nv_exit(int status) __attribute__((noreturn));

/* Map common names to our implementations */
#define malloc(sz) nv_malloc(sz)
#define free(p) nv_free(p)
#define realloc(p, sz) nv_realloc(p, sz)
#define calloc(n, s) nv_calloc(n, s)

#define memcpy(d, s, n) nv_memcpy(d, s, n)
#define memset(s, c, n) nv_memset(s, c, n)
#define memcmp(s1, s2, n) nv_memcmp(s1, s2, n)

#define strlen(s) nv_strlen(s)
#define strcmp(a,b) nv_strcmp(a,b)
#define strncmp(a,b,n) nv_strncmp(a,b,n)
#define strcpy(d,s) nv_strcpy(d,s)
#define strncpy(d,s,n) nv_strncpy(d,s,n)
#define strdup(s) nv_strdup(s)
#define strstr(h,n) nv_strstr(h,n)

#define printf(...) nv_printf(__VA_ARGS__)
#define fprintf(fd, ...) nv_fprintf(fd, __VA_ARGS__)

#define atoi(s) nv_atoi(s)
#define sleep(s) nv_sleep(s)

/* Map stdio names to our syscall-backed replacements */
#define fopen(p,m) nv_fopen(p,m)
#define fgets(s,n,fp) nv_fgets(s,n,fp)
#define fclose(fp) nv_fclose(fp)
#define exit(s) nv_exit(s)

#endif /* COMPAT_H */
