#include "aes.h"

#include <stdio.h>
#include <string.h>

static int tests_run = 0, tests_passed = 0, tests_failed = 0;

#define ASSERT_BYTES_EQ(exp, act, n, msg) do {                          \
    tests_run++;                                                        \
    if (memcmp((exp), (act), (n)) == 0) { tests_passed++; }             \
    else {                                                              \
        tests_failed++;                                                 \
        fprintf(stderr, "  FAIL [%s:%d] %s\n    expected: ",            \
                __FILE__, __LINE__, msg);                               \
        for (size_t _i = 0; _i < (n); _i++)                             \
            fprintf(stderr, "%02x", ((const uint8_t*)(exp))[_i]);       \
        fprintf(stderr, "\n    actual:   ");                            \
        for (size_t _i = 0; _i < (n); _i++)                             \
            fprintf(stderr, "%02x", ((const uint8_t*)(act))[_i]);       \
        fprintf(stderr, "\n");                                          \
    }                                                                   \
} while (0)

#define RUN_TEST(fn) do { printf("  %s ...\n", #fn); fn(); } while (0)

/* FIPS-197 Appendix C.1 test vector. */
static void test_fips197_vector(void) {
    uint8_t key[16] = {
        0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,
        0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f
    };
    uint8_t plain[16] = {
        0x00,0x11,0x22,0x33,0x44,0x55,0x66,0x77,
        0x88,0x99,0xaa,0xbb,0xcc,0xdd,0xee,0xff
    };
    uint8_t cipher[16] = {
        0x69,0xc4,0xe0,0xd8,0x6a,0x7b,0x04,0x30,
        0xd8,0xcd,0xb7,0x80,0x70,0xb4,0xc5,0x5a
    };

    Aes128Ctx ctx;
    aes128_init(&ctx, key);
    uint8_t got[16];
    aes128_encrypt_block(&ctx, plain, got);
    ASSERT_BYTES_EQ(cipher, got, 16, "encrypt");
    aes128_decrypt_block(&ctx, cipher, got);
    ASSERT_BYTES_EQ(plain, got, 16, "decrypt");
}

static void test_cbc_roundtrip(void) {
    uint8_t key[16] = { 1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16 };
    uint8_t iv[16]  = { 0xA0,0xA1,0xA2,0xA3,0xA4,0xA5,0xA6,0xA7,
                        0xA8,0xA9,0xAA,0xAB,0xAC,0xAD,0xAE,0xAF };
    uint8_t plain[64];
    for (int i = 0; i < 64; i++) plain[i] = (uint8_t)(i * 7 + 3);
    uint8_t cipher[64], got[64];
    aes128_cbc_encrypt(key, iv, plain, cipher, 64);
    aes128_cbc_decrypt(key, iv, cipher, got, 64);
    ASSERT_BYTES_EQ(plain, got, 64, "cbc roundtrip");
}

int main(void) {
    printf("test_aes\n");
    RUN_TEST(test_fips197_vector);
    RUN_TEST(test_cbc_roundtrip);
    printf("  %d run, %d passed, %d failed\n",
           tests_run, tests_passed, tests_failed);
    return tests_failed == 0 ? 0 : 1;
}
