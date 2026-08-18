#ifndef __NVRAM_FAKER_H__
#define __NVRAM_FAKER_H__

/* Logging and export helpers */
#ifndef DEBUG_PRINTF
#ifdef DEBUG
#define DEBUG_PRINTF(...) printf(__VA_ARGS__)
#else
#define DEBUG_PRINTF(...) ((void)0)
#endif
#endif

#ifndef LOG_PRINTF
#define LOG_PRINTF(...) printf(__VA_ARGS__)
#endif

#ifndef EXPORT
#define EXPORT
#endif

char *nvram_get(const char *key);
int nvram_set(const char *key, const char *value);

/* Simple wrapper for strcmp used in original code */
int nvram_faker_strcmp(const char *a, const char *b);

#endif /* __NVRAM_FAKER_H__ */