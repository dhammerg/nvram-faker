#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "nvram-faker.h"
#include "nvram-faker-internal.h"

static int passed = 0;
static int failed = 0;

#define TEST(cond, msg)                         \
    do {                                        \
        if (cond) {                             \
            printf("[PASS] %s\n", msg);         \
            passed++;                           \
        } else {                                \
            printf("[FAIL] %s\n", msg);         \
            failed++;                           \
        }                                       \
    } while (0)

int main(void)
{
    char *v;

    printf("=== nvram-faker tests ===\n\n");

    initialize_ini();

    /* 1. Get existing key */
    v = nvram_get("lan_ipaddr");
    TEST(v && strcmp(v, "192.168.127.141") == 0,
         "get existing key");
    free(v);

    /* 2. Get missing key */
    v = nvram_get("does_not_exist");
    TEST(v == NULL, "get missing key");

    /* 3. Update existing key */
    TEST(nvram_set("lan_ipaddr", "10.0.0.1") == 0,
         "set existing key");

    v = nvram_get("lan_ipaddr");
    TEST(v && strcmp(v, "10.0.0.1") == 0,
         "verify updated key");
    free(v);

    /* 4. Add new key */
    TEST(nvram_set("test_variable", "hello") == 0,
         "add new key");

    v = nvram_get("test_variable");
    TEST(v && strcmp(v, "hello") == 0,
         "verify new key");
    free(v);

    /* 5. Update same key */
    TEST(nvram_set("test_variable", "world") == 0,
         "update same key");

    v = nvram_get("test_variable");
    TEST(v && strcmp(v, "world") == 0,
         "verify second update");
    free(v);

    /* 6. Empty value */
    TEST(nvram_set("empty", "") == 0,
         "set empty value");

    v = nvram_get("empty");
    TEST(v && strcmp(v, "") == 0,
         "verify empty value");
    free(v);

    /* 7. Invalid arguments */
    TEST(nvram_set(NULL, "value") != 0,
         "reject NULL key");

    TEST(nvram_set("key", NULL) != 0,
         "reject NULL value");

    /* Cleanup */
    end();

    printf("\n=== Results ===\n");
    printf("Passed: %d\n", passed);
    printf("Failed: %d\n", failed);

    return failed ? 1 : 0;
}