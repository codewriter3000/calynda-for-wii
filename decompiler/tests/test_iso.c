#include "iso.h"

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

#define ASSERT_EQ_STR(expected, actual, msg) do {                       \
    tests_run++;                                                        \
    if (strcmp((expected), (actual)) == 0) { tests_passed++; }          \
    else { tests_failed++;                                              \
        fprintf(stderr, "  FAIL [%s:%d] %s: expected \"%s\", got \"%s\"\n", \
                __FILE__, __LINE__, msg, (expected), (actual)); }       \
} while (0)

#define RUN_TEST(fn) do { printf("  %s ...\n", #fn); fn(); } while (0)

static void put_u32_be(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)(v >> 24); p[1] = (uint8_t)(v >> 16);
    p[2] = (uint8_t)(v >>  8); p[3] = (uint8_t)(v);
}

static FILE *open_tmp_with(const uint8_t *buf, size_t n) {
    FILE *fp = tmpfile();
    if (!fp) return NULL;
    fwrite(buf, 1, n, fp);
    fflush(fp);
    return fp;
}

static void test_gc_probe(void) {
    uint8_t img[0x440];
    memset(img, 0, sizeof(img));
    memcpy(img, "GAMEID", 6);
    memcpy(img + 0x20, "My GC Game", 10);
    put_u32_be(img + 0x1C,  0xC2339F3Du);
    put_u32_be(img + 0x420, 0x00010000u);

    FILE *fp = open_tmp_with(img, sizeof(img));
    IsoInfo info;
    ASSERT_EQ_INT(ISO_OK, iso_probe(fp, &info), "probe gc");
    ASSERT_EQ_INT(ISO_FORMAT_GAMECUBE, info.format, "gc format");
    ASSERT_EQ_STR("GAMEID", info.game_id, "gc id");
    ASSERT_EQ_STR("My GC Game", info.game_title, "gc title");
    ASSERT_EQ_INT(0x00010000, info.gc_dol_offset, "gc dol offset");
    fclose(fp);
}

static void test_wii_probe_reports_encrypted(void) {
    /* Image must extend past the partition records. */
    size_t n = 0x40000 + 0x40;
    uint8_t *img = (uint8_t *)calloc(1, n);
    memcpy(img, "RSPE01", 6);            /* Wii Sports game id, just as a label */
    memcpy(img + 0x20, "Wii Sports", 10);
    put_u32_be(img + 0x18, 0x5D1C9EA3u); /* Wii magic */
    /* First entry: 1 partition, table immediately after info table. */
    put_u32_be(img + 0x40000, 1u);
    put_u32_be(img + 0x40004, (0x40020u) >> 2);
    /* Partition record: offset=0x50000 (>>2), type=0 (data) */
    put_u32_be(img + 0x40020, (0x50000u) >> 2);
    put_u32_be(img + 0x40024, 0u);

    FILE *fp = open_tmp_with(img, n);
    IsoInfo info;
    ASSERT_EQ_INT(ISO_OK, iso_probe(fp, &info), "probe wii");
    ASSERT_EQ_INT(ISO_FORMAT_WII, info.format, "wii format");
    ASSERT_EQ_INT(1, info.wii_partition_count, "wii partition count");
    ASSERT_EQ_INT(0x50000, info.wii_partitions[0].offset, "wii part offset");

    /* GC DOL extraction must refuse encrypted Wii discs. */
    uint8_t *buf = NULL; size_t sz = 0;
    ASSERT_EQ_INT(ISO_ERR_ENCRYPTED,
                  iso_extract_gc_dol(fp, &info, &buf, &sz),
                  "extract on wii rejected");
    fclose(fp);
    free(img);
}

static void test_unknown_format(void) {
    uint8_t img[0x440];
    memset(img, 0, sizeof(img));
    FILE *fp = open_tmp_with(img, sizeof(img));
    IsoInfo info;
    ASSERT_EQ_INT(ISO_ERR_UNKNOWN_FORMAT, iso_probe(fp, &info), "unknown");
    fclose(fp);
}

int main(void) {
    printf("test_iso\n");
    RUN_TEST(test_gc_probe);
    RUN_TEST(test_wii_probe_reports_encrypted);
    RUN_TEST(test_unknown_format);
    printf("  %d run, %d passed, %d failed\n",
           tests_run, tests_passed, tests_failed);
    return tests_failed == 0 ? 0 : 1;
}
