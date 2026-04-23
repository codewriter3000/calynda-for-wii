/*
 * Build a synthetic Wii-style encrypted partition in memory and
 * verify that WiiPartitionReader round-trips the decrypted bytes.
 *
 * This does not use any Nintendo keys: the "common key" and title key
 * are arbitrary bytes chosen by the test. It exercises the exact
 * ticket unwrap + group-decrypt paths used on real media.
 */
#include "aes.h"
#include "wii_crypto.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int tests_run = 0, tests_passed = 0, tests_failed = 0;

#define ASSERT_EQ_INT(expected, actual, msg) do {                       \
    tests_run++;                                                        \
    long _e = (long)(expected), _a = (long)(actual);                    \
    if (_e == _a) { tests_passed++; }                                   \
    else { tests_failed++;                                              \
        fprintf(stderr, "  FAIL [%s:%d] %s: expected %ld, got %ld\n",  \
                __FILE__, __LINE__, msg, _e, _a); }                     \
} while (0)

#define ASSERT_BYTES_EQ(exp, act, n, msg) do {                          \
    tests_run++;                                                        \
    if (memcmp((exp), (act), (n)) == 0) { tests_passed++; }             \
    else { tests_failed++;                                              \
        fprintf(stderr, "  FAIL [%s:%d] %s\n", __FILE__, __LINE__, msg);\
    }                                                                   \
} while (0)

#define RUN_TEST(fn) do { printf("  %s ...\n", #fn); fn(); } while (0)

static void put_u32_be(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)(v >> 24); p[1] = (uint8_t)(v >> 16);
    p[2] = (uint8_t)(v >>  8); p[3] = (uint8_t)(v);
}

/*
 * Construct a partition image with:
 *   - A ticket whose title-key slot holds an encrypted form of the
 *     given title key (encrypted under `common_key` with IV = title_id
 *     padded to 16 bytes).
 *   - A data region containing 2 encrypted groups (0x8000 bytes each)
 *     that together carry a 2 * 0x7C00 byte plaintext payload.
 */
#define PARTITION_OFFSET  0x00050000u
#define DATA_OFFSET       0x00020000u  /* relative to partition */
#define GROUPS            2u
#define PLAINTEXT_SIZE    (GROUPS * WII_BLOCK_DATA_SIZE)

static void build_fixture(uint8_t *iso, size_t iso_size,
                          const uint8_t common_key[16],
                          const uint8_t title_key[16],
                          const uint8_t title_id[8],
                          const uint8_t *plain,
                          size_t plain_size) {
    memset(iso, 0, iso_size);

    /* --- Ticket ---------------------------------------------------- */
    uint8_t *part = iso + PARTITION_OFFSET;
    memcpy(part + 0x01DC, title_id, 8);

    uint8_t tk_iv[16]; memset(tk_iv, 0, 16);
    memcpy(tk_iv, title_id, 8);
    aes128_cbc_encrypt(common_key, tk_iv,
                       title_key, part + 0x01BF, 16);

    /* Data offset (shifted by 2). */
    put_u32_be(part + 0x02B8, (uint32_t)(DATA_OFFSET >> 2));

    /* --- Encrypted data groups ------------------------------------- */
    for (uint32_t g = 0; g < GROUPS; g++) {
        uint8_t hash_plain[WII_BLOCK_HASH_SIZE];
        memset(hash_plain, 0, sizeof(hash_plain));
        /* Pick a non-zero IV for user data and store it at 0x3D0. */
        for (int i = 0; i < 16; i++) {
            hash_plain[0x3D0 + i] = (uint8_t)(0x10 * (g + 1) + i);
        }

        uint8_t *group_base = part + DATA_OFFSET + (size_t)g * WII_BLOCK_SIZE;

        uint8_t hash_iv[16]; memset(hash_iv, 0, 16);
        aes128_cbc_encrypt(title_key, hash_iv,
                           hash_plain, group_base, WII_BLOCK_HASH_SIZE);

        uint8_t data_iv[16]; memcpy(data_iv, hash_plain + 0x3D0, 16);
        size_t off = (size_t)g * WII_BLOCK_DATA_SIZE;
        aes128_cbc_encrypt(title_key, data_iv,
                           plain + off,
                           group_base + WII_BLOCK_HASH_SIZE,
                           WII_BLOCK_DATA_SIZE);

        (void)plain_size;
    }
}

static void test_partition_roundtrip(void) {
    uint8_t common_key[16]; for (int i=0;i<16;i++) common_key[i] = (uint8_t)(0xC0 + i);
    uint8_t title_key[16];  for (int i=0;i<16;i++) title_key[i]  = (uint8_t)(0xA0 + i);
    uint8_t title_id[8]   = { 0x00,0x01,0x00,0x00, 'R','S','P','E' };

    size_t iso_size = PARTITION_OFFSET + DATA_OFFSET + GROUPS * WII_BLOCK_SIZE;
    uint8_t *iso = (uint8_t *)malloc(iso_size);

    uint8_t *plain = (uint8_t *)malloc(PLAINTEXT_SIZE);
    for (size_t i = 0; i < PLAINTEXT_SIZE; i++) {
        plain[i] = (uint8_t)((i * 131u) ^ (i >> 3));
    }

    build_fixture(iso, iso_size, common_key, title_key, title_id,
                  plain, PLAINTEXT_SIZE);

    FILE *fp = tmpfile();
    fwrite(iso, 1, iso_size, fp); fflush(fp);

    WiiPartitionReader r;
    ASSERT_EQ_INT(WII_OK,
                  wii_partition_open(fp, PARTITION_OFFSET, common_key, &r),
                  "open partition");
    ASSERT_BYTES_EQ(title_key, r.title_key, 16, "title key unwrap");

    /* Read the full plaintext payload across the group boundary. */
    uint8_t *got = (uint8_t *)malloc(PLAINTEXT_SIZE);
    ASSERT_EQ_INT(WII_OK,
                  wii_partition_read(&r, 0, got, PLAINTEXT_SIZE),
                  "read all");
    ASSERT_BYTES_EQ(plain, got, PLAINTEXT_SIZE, "plaintext bytes");

    /* Read that straddles the group boundary precisely. */
    uint64_t boundary = WII_BLOCK_DATA_SIZE - 7;
    uint8_t small[32];
    ASSERT_EQ_INT(WII_OK,
                  wii_partition_read(&r, boundary, small, 32),
                  "read across boundary");
    ASSERT_BYTES_EQ(plain + boundary, small, 32, "boundary bytes");

    free(got); free(plain); free(iso); fclose(fp);
}

int main(void) {
    printf("test_wii_crypto\n");
    RUN_TEST(test_partition_roundtrip);
    printf("  %d run, %d passed, %d failed\n",
           tests_run, tests_passed, tests_failed);
    return tests_failed == 0 ? 0 : 1;
}
