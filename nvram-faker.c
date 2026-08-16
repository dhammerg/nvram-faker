#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "nvram-faker.h"
//include before ini.h to override ini.h defaults
#include "nvram-faker-internal.h"
#include "ini.h"

#define RED_ON "\033[22;31m"
#define RED_OFF "\033[22;00m"
#define DEFAULT_KV_PAIR_LEN 1024

static int kv_count=0;
static int key_value_pair_len=DEFAULT_KV_PAIR_LEN;
static char **key_value_pairs=NULL;

static int ini_handler(void *user, const char *section, const char *name, const char *value)
{
    char **kv;

    if (user == NULL ||
        section == NULL ||
        name == NULL ||
        value == NULL)
    {
        DEBUG_PRINTF("bad parameter to ini_handler\n");
        return 0;
    }

    kv = *((char ***)user);

    if (kv == NULL)
    {
        LOG_PRINTF("kv is NULL\n");
        return 0;
    }

    DEBUG_PRINTF("kv_count: %d, key_value_pair_len: %d\n",
                 kv_count,
                 key_value_pair_len);

    if (kv_count + 2 > key_value_pair_len)
    {
        int old_kv_len;
        char **new_kv;

        old_kv_len = key_value_pair_len;
        key_value_pair_len *= 2;

        new_kv = realloc(kv, key_value_pair_len * sizeof(*new_kv));

        if (new_kv == NULL)
        {
            LOG_PRINTF("Failed to reallocate key value array.\n");
            key_value_pair_len = old_kv_len;
            return 0;
        }

        kv = new_kv;
        *(char ***)user = kv;
    }
    DEBUG_PRINTF("Got %s:%s\n", name, value);

    kv[kv_count++] = strdup(name);
    kv[kv_count++] = strdup(value);

    return 1;
}

void initialize_ini(void)
{
    int ret;
    DEBUG_PRINTF("Initializing.\n");
    if (NULL == key_value_pairs)
    {
        key_value_pairs=malloc(key_value_pair_len * sizeof(char **));
    }
    if(NULL == key_value_pairs)
    {
        LOG_PRINTF("Failed to allocate memory for key value array. Terminating.\n");
        exit(1);
    }
    
    ret = ini_parse(INI_FILE_PATH,ini_handler,(void *)&key_value_pairs);
    if (0 != ret)
    {
        LOG_PRINTF("ret from ini_parse was: %d\n",ret);
        LOG_PRINTF("INI parse failed. Terminating\n");
        free(key_value_pairs);
        key_value_pairs=NULL;
        exit(1);
    }else
    {
        DEBUG_PRINTF("ret from ini_parse was: %d\n",ret);
    }
    
    return;
    
}

void end(void)
{
    int i;
    for (i=0;i<kv_count;i++)
    {
        free(key_value_pairs[i]);
    }
    free(key_value_pairs);
    key_value_pairs=NULL;
    
    return;
}

char *nvram_get(const char *key)
{
    int i;
    int found=0;
    char *value;
    char *ret;
    for(i=0;i<kv_count;i+=2)
    {
        if(strcmp(key,key_value_pairs[i]) == 0)
        {
            LOG_PRINTF("%s=%s\n",key,key_value_pairs[i+1]);
            found = 1;
            value=key_value_pairs[i+1];
            break;
        }
    }

    ret = NULL;
    if(!found)
    {
            LOG_PRINTF( RED_ON"%s=Unknown\n"RED_OFF,key);
    }else
    {

            ret=strdup(value);
    }
    return ret;
}


int nvram_set(const char *key, const char *value)
{
    int i;
    char *new_value;
    char *new_key;

    if (key == NULL || value == NULL)
    {
        LOG_PRINTF("nvram_set: invalid argument\n");
        return -1;
    }

    if (key_value_pairs == NULL)
    {
        LOG_PRINTF("nvram_set: NVRAM is not initialized\n");
        return -1;
    }

    /*
     * Check whether the key already exists.
     * If it does, replace only its value.
     */
    for (i = 0; i < kv_count; i += 2)
    {
        if (strcmp(key, key_value_pairs[i]) == 0)
        {
            new_value = strdup(value);

            if (new_value == NULL)
            {
                LOG_PRINTF("nvram_set: failed to allocate value\n");
                return -1;
            }

            free(key_value_pairs[i + 1]);
            key_value_pairs[i + 1] = new_value;

            LOG_PRINTF("nvram_set: updated %s=%s\n", key, value);

            return 0;
        }
    }

    /*
     * Key does not exist.
     * We need two new entries:
     *
     *   key_value_pairs[kv_count]     = key
     *   key_value_pairs[kv_count + 1] = value
     */
    if (kv_count + 2 > key_value_pair_len)
    {
        int new_len;
        char **new_pairs;

        new_len = key_value_pair_len * 2;

        /*
         * Make sure the new array is large enough even if the
         * current array is very small.
         */
        while (kv_count + 2 > new_len)
        {
            new_len *= 2;
        }

        new_pairs = realloc(key_value_pairs,
                            new_len * sizeof(*new_pairs));

        if (new_pairs == NULL)
        {
            LOG_PRINTF("nvram_set: failed to resize key/value array\n");
            return -1;
        }

        key_value_pairs = new_pairs;
        key_value_pair_len = new_len;

        DEBUG_PRINTF("nvram_set: resized array to %d entries\n",
                     key_value_pair_len);
    }

    /*
     * Allocate the key first.
     */
    new_key = strdup(key);

    if (new_key == NULL)
    {
        LOG_PRINTF("nvram_set: failed to allocate key\n");
        return -1;
    }

    /*
     * Allocate the value.
     */
    new_value = strdup(value);

    if (new_value == NULL)
    {
        LOG_PRINTF("nvram_set: failed to allocate value\n");
        free(new_key);
        return -1;
    }

    /*
     * Add the new key/value pair.
     */
    key_value_pairs[kv_count] = new_key;
    key_value_pairs[kv_count + 1] = new_value;

    kv_count += 2;

    LOG_PRINTF("nvram_set: added %s=%s\n", key, value);

    return 0;
}