#include "dol.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/*  Minimal test harness (style matches the compiler tests)           */
/* ------------------------------------------------------------------ */

static int tests_run    = 0;
static int tests_passed = 0;
static int tests_failed = 0;

#define ASSERT_TRUE(cond, msg) do {                                     \
    tests_run++;                                                        \
    if (cond) {                                                         \
        tests_passed++;                                                 \
    } else {                                                            \
        tests_failed++;                                                 \
        fprintf(stderr, "  FAIL [%s:%d] %s\n", __FILE__, __LINE__, msg);\
    }                                                                   \
} while (0)

#define ASSERT_EQ_U32(expected, actual, msg) do {                       \
    tests_run++;                                                        \
    uint32_t _e = (uint32_t)(expected), _a = (uint32_t)(actual);        \
    if (_e == _a) {                                                     \
        tests_passed++;                                                 \
    } else {                                                            \
        tests_failed++;                                                 \
        fprintf(stderr,                                                 \
                "  FAIL [%s:%d] %s: expected 0x%08x, got 0x%08x\n",     \
                __FILE__, __LINE__, msg, _e, _a);                       \
    }                                                                   \
} while (0)

#define ASSERT_EQ_INT(expected, actual, msg) do {                       \
    tests_run++;                                                        \
    long _e = (long)(expected), _a = (long)(actual);                    \
    if (_e == _a) {                                                     \
        tests_passed++;                                                 \
    } else {                                                            \
        tests_failed++;                                                 \
        fprintf(stderr, "  FAIL [%s:%d] %s: expected %ld, got %ld\n",  \
                __FILE__, __LINE__, msg, _e, _a);                       \
    }                                                                   \
} while (0)

#define RUN_TEST(fn) do { printf("  %s ...\n", #fn); fn(); } while (0)

/* ------------------------------------------------------------------ */
/*  Fixture builder                                                    */
/* ------------------------------------------------------------------ */

static void put_u32_be(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)(v >> 24);
    p[1] = (uint8_t)(v >> 16);
    p[2] = (uint8_t)(v >>  8);
    p[3] = (uint8_t)(v);
}

/*
 * Build a minimal but valid DOL in memory:
 *   - 1 text section at file 0x100, vaddr 0x80003100, size 0x40
 *   - 1 data section at file 0x140, vaddr 0x80003200, size 0x20
 *   - BSS addr 0x80004000, size 0x100
 *   - Entrypoint 0x80003100 (start of text0)
 * Total file size: 0x160 bytes.
 */
#define FIXTURE_TEXT0_OFF   0x100u
#define FIXTURE_TEXT0_ADDR  0x80003100u
#define FIXTURE_TEXT0_SIZE  0x40u
#define FIXTURE_DATA0_OFF   0x140u
#define FIXTURE_DATA0_ADDR  0x80003200u
#define FIXTURE_DATA0_SIZE  0x20u
#define FIXTURE_BSS_ADDR    0x80004000u
#define FIXTURE_BSS_SIZE    0x100u
#define FIXTURE_ENTRY       0x80003100u
#define FIXTURE_SIZE        0x160u

static void build_fixture(uint8_t *buf) {
    memset(buf, 0, FIXTURE_SIZE);
    put_u32_be(buf + 0x00, FIXTURE_TEXT0_OFF);
    put_u32_be(buf + 0x48, FIXTURE_TEXT0_ADDR);
    put_u32_be(buf + 0x90, FIXTURE_TEXT0_SIZE);
    put_u32_be(buf + 0x1C, FIXTURE_DATA0_OFF);
    put_u32_be(buf + 0x64, FIXTURE_DATA0_ADDR);
    put_u32_be(buf + 0xAC, FIXTURE_DATA0_SIZE);
    put_u32_be(buf + 0xD8, FIXTURE_BSS_ADDR);
    put_u32_be(buf + 0xDC, FIXTURE_BSS_SIZE);
    put_u32_be(buf + 0xE0, FIXTURE_ENTRY);
    /* Put a recognizable marker at the start of text0. */
    put_u32_be(buf + FIXTURE_TEXT0_OFF, 0xDEADBEEFu);
}

/* ------------------------------------------------------------------ */
/*  Tests                                                              */
/* ------------------------------------------------------------------ */

static void test_parse_valid_fixture(void) {
    uint8_t buf[FIXTURE_SIZE];
    build_fixture(buf);

    DolImage img;
    DolStatus st = dol_parse(buf, sizeof(buf), &img);
    ASSERT_EQ_INT(DOL_OK, st, "parse valid fixture");
    ASSERT_EQ_U32(FIXTURE_ENTRY,    img.entrypoint, "entrypoint");
    ASSERT_EQ_U32(FIXTURE_BSS_ADDR, img.bss_addr,   "bss addr");
    ASSERT_EQ_U32(FIXTURE_BSS_SIZE, img.bss_size,   "bss size");
    ASSERT_EQ_INT(2, img.section_count, "non-empty section count");

    ASSERT_EQ_INT(DOL_SECTION_TEXT, img.sections[0].kind, "sec 0 kind");
    ASSERT_EQ_U32(FIXTURE_TEXT0_ADDR, img.sections[0].virtual_addr, "sec 0 vaddr");
    ASSERT_EQ_U32(FIXTURE_TEXT0_SIZE, img.sections[0].size, "sec 0 size");

    ASSERT_EQ_INT(DOL_SECTION_DATA, img.sections[1].kind, "sec 1 kind");
    ASSERT_EQ_U32(FIXTURE_DATA0_ADDR, img.sections[1].virtual_addr, "sec 1 vaddr");
}

static void test_too_small(void) {
    uint8_t buf[16] = {0};
    DolImage img;
    DolStatus st = dol_parse(buf, sizeof(buf), &img);
    ASSERT_EQ_INT(DOL_ERR_TOO_SMALL, st, "reject undersized buffer");
}

static void test_null_args(void) {
    DolImage img;
    ASSERT_EQ_INT(DOL_ERR_NULL_ARG, dol_parse(NULL, 0x100, &img), "null bytes");
    uint8_t buf[FIXTURE_SIZE];
    build_fixture(buf);
    ASSERT_EQ_INT(DOL_ERR_NULL_ARG, dol_parse(buf, sizeof(buf), NULL), "null out");
}

static void test_section_out_of_bounds(void) {
    uint8_t buf[FIXTURE_SIZE];
    build_fixture(buf);
    /* Push text0 size past EOF. */
    put_u32_be(buf + 0x90, 0x1000u);
    DolImage img;
    DolStatus st = dol_parse(buf, sizeof(buf), &img);
    ASSERT_EQ_INT(DOL_ERR_BAD_SECTION, st, "reject out-of-bounds section");
}

static void test_misaligned_rejected(void) {
    uint8_t buf[FIXTURE_SIZE];
    build_fixture(buf);
    /* Offset 0x101 is not 4-byte aligned. */
    put_u32_be(buf + 0x00, 0x101u);
    DolImage img;
    DolStatus st = dol_parse(buf, sizeof(buf), &img);
    ASSERT_EQ_INT(DOL_ERR_MISALIGNED, st, "reject misaligned offset");
}

static void test_entrypoint_must_be_in_text(void) {
    uint8_t buf[FIXTURE_SIZE];
    build_fixture(buf);
    /* Point entry at the data section. */
    put_u32_be(buf + 0xE0, FIXTURE_DATA0_ADDR);
    DolImage img;
    DolStatus st = dol_parse(buf, sizeof(buf), &img);
    ASSERT_EQ_INT(DOL_ERR_NO_ENTRYPOINT, st, "entrypoint in data rejected");

    /* And an entry that lands in BSS (unmapped) must also be rejected. */
    build_fixture(buf);
    put_u32_be(buf + 0xE0, FIXTURE_BSS_ADDR);
    st = dol_parse(buf, sizeof(buf), &img);
    ASSERT_EQ_INT(DOL_ERR_NO_ENTRYPOINT, st, "entrypoint in bss rejected");
}

static void test_vaddr_translation(void) {
    uint8_t buf[FIXTURE_SIZE];
    build_fixture(buf);
    DolImage img;
    ASSERT_EQ_INT(DOL_OK, dol_parse(buf, sizeof(buf), &img), "parse");

    size_t remaining = 0;
    const uint8_t *p = dol_vaddr_to_ptr(&img, FIXTURE_TEXT0_ADDR, &remaining);
    ASSERT_TRUE(p != NULL, "vaddr at text start resolves");
    ASSERT_EQ_INT(FIXTURE_TEXT0_SIZE, remaining, "remaining at text start");
    ASSERT_EQ_U32(0xDEADBEEFu,
                  ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
                  ((uint32_t)p[2] <<  8) |  (uint32_t)p[3],
                  "text0 first word");

    /* An address past the last section maps to NULL. */
    const uint8_t *q = dol_vaddr_to_ptr(&img, 0x90000000u, NULL);
    ASSERT_TRUE(q == NULL, "unmapped vaddr returns NULL");

    /* BSS is not backed by file bytes; must also return NULL. */
    const uint8_t *b = dol_vaddr_to_ptr(&img, FIXTURE_BSS_ADDR, NULL);
    ASSERT_TRUE(b == NULL, "bss vaddr returns NULL");
}

static void test_empty_sections_skipped(void) {
    uint8_t buf[FIXTURE_SIZE];
    build_fixture(buf);
    DolImage img;
    ASSERT_EQ_INT(DOL_OK, dol_parse(buf, sizeof(buf), &img), "parse");
    /* Only the 2 non-empty sections should be populated. */
    ASSERT_EQ_INT(2, img.section_count, "empty section slots skipped");
}

int main(void) {
    printf("test_dol\n");
    RUN_TEST(test_parse_valid_fixture);
    RUN_TEST(test_too_small);
    RUN_TEST(test_null_args);
    RUN_TEST(test_section_out_of_bounds);
    RUN_TEST(test_misaligned_rejected);
    RUN_TEST(test_entrypoint_must_be_in_text);
    RUN_TEST(test_vaddr_translation);
    RUN_TEST(test_empty_sections_skipped);
    printf("  %d run, %d passed, %d failed\n",
           tests_run, tests_passed, tests_failed);
    return tests_failed == 0 ? 0 : 1;
}
