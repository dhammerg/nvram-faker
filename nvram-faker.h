#ifndef __NVRAM_FAKER_H__
#define __NVRAM_FAKER_H__

char *nvram_get(const char *key);
int nvram_set(const char *key, const char *value);

#endif /* __NVRAM_FAKER_H__ */