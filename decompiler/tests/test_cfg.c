/*
 * Tests for the CFG builder. We hand-craft a tiny DOL image in memory
 * with one text section containing precisely the instruction patterns
 * we want to exercise, then feed it through cfg_build.
 */
#include "cli/cfg.h"
#include "dol.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int tests_run = 0, tests_passed = 0, tests_failed = 0;

#define ASSERT_TRUE(cond, msg) do {                                   \
    tests_run++;                                                      \
    if (cond) tests_passed++;                                         \
    else { tests_failed++;                                            \
        fprintf(stderr, "  FAIL [%s:%d] %s\n",                        \
                __FILE__, __LINE__, msg); }                           \
} while (0)

#define ASSERT_EQ_INT(e, a, msg) do {                                 \
    tests_run++;                                                      \
    long _e = (long)(e), _a = (long)(a);                              \
    if (_e == _a) tests_passed++;                                     \
    else { tests_failed++;                                            \
        fprintf(stderr, "  FAIL [%s:%d] %s: expected %ld, got %ld\n", \
                __FILE__, __LINE__, msg, _e, _a); }                   \
} while (0)

#define RUN_TEST(fn) do { printf("  %s ...\n", #fn); fn(); } while (0)

/* ----- build a DOL image around a single text section ----- */

static void wbe32(uint8_t *p, uint32_t v) {
    p[0] = v >> 24; p[1] = v >> 16; p[2] = v >> 8; p[3] = v;
}

/* text0 at file offset 0x100, virtual address 0x80004000. BSS empty. */
static uint8_t *make_dol(const uint32_t *words, size_t n, size_t *out_size) {
    size_t text_bytes = n * 4u;
    size_t total = 0x100 + text_bytes;
    uint8_t *buf = (uint8_t *)calloc(total, 1);
    if (!buf) return NULL;

    /* text0 offset / vaddr / size */
    wbe32(buf + 0x00, 0x100);
    wbe32(buf + 0x48, 0x80004000u);
    wbe32(buf + 0x90, (uint32_t)text_bytes);
    /* entrypoint */
    wbe32(buf + 0xE0, 0x80004000u);

    for (size_t i = 0; i < n; i++)
        wbe32(buf + 0x100 + i * 4u, words[i]);

    *out_size = total;
    return buf;
}

/* ----- branch / insn encoding helpers ----- */

static uint32_t enc_b(uint32_t from, uint32_t to, int lk) {
    int32_t disp = (int32_t)(to - from);
    return (18u << 26) | ((uint32_t)disp & 0x03FFFFFCu) | (lk ? 1u : 0u);
}

/* bc 12, 0, target  => branch if cr0[lt] set.  BO=12, BI=0. */
static uint32_t enc_bc(uint32_t from, uint32_t to, uint32_t bo, uint32_t bi) {
    int32_t disp = (int32_t)(to - from);
    return (16u << 26) | ((bo & 0x1Fu) << 21) | ((bi & 0x1Fu) << 16) |
           ((uint32_t)disp & 0xFFFCu);
}

#define BLR     0x4E800020u
#define NOP     0x60000000u

/* ----- tests ----- */

static void test_linear_nop_then_blr(void) {
    uint32_t w[] = { NOP, NOP, BLR };
    size_t sz = 0;
    uint8_t *buf = make_dol(w, 3, &sz);
    DolImage img;
    ASSERT_EQ_INT(0, dol_parse(buf, sz, &img), "dol parse");
    Cfg cfg;
    ASSERT_EQ_INT(0, cfg_build(&img, 0x80004000u, 0x80004000u + 12, &cfg),
                  "build");
    ASSERT_EQ_INT(1, (int)cfg.block_count, "one block");
    ASSERT_EQ_INT(1, (int)cfg.edge_count, "one edge (return)");
    ASSERT_TRUE(cfg.edges[0].kind == CFG_EDGE_RETURN, "return edge");
    ASSERT_EQ_INT(0, (int)cfg.is_irreducible, "reducible");
    cfg_free(&cfg);
    free(buf);
}

static void test_forward_branch_splits_block(void) {
    /* 0: nop
     * 4: b +8         (-> 12)
     * 8: nop           (unreachable, but still a block)
     * 12: blr
     */
    uint32_t base = 0x80004000u;
    uint32_t w[] = {
        NOP,
        enc_b(base + 4, base + 12, 0),
        NOP,
        BLR,
    };
    size_t sz; uint8_t *buf = make_dol(w, 4, &sz);
    DolImage img; dol_parse(buf, sz, &img);
    Cfg cfg;
    ASSERT_EQ_INT(0, cfg_build(&img, base, base + 16, &cfg), "build");
    /* B0 = {0,4}, B1 = {8}, B2 = {12}. */
    ASSERT_EQ_INT(3, (int)cfg.block_count, "three blocks");
    /* B0 ends with unconditional b to B2. */
    int saw_uncond_to_b2 = 0;
    for (size_t i = 0; i < cfg.edge_count; i++) {
        if (cfg.edges[i].from == 0 &&
            cfg.edges[i].kind == CFG_EDGE_UNCOND &&
            cfg.edges[i].to   == 2) saw_uncond_to_b2 = 1;
    }
    ASSERT_TRUE(saw_uncond_to_b2, "B0 -> B2 uncond");
    cfg_free(&cfg);
    free(buf);
}

static void test_if_else_converges(void) {
    /* 0:  bc    lt, cr0, +12     (taken -> 12)
     * 4:  nop
     * 8:  b     +8               (-> 16)
     *12:  nop
     *16:  blr
     */
    uint32_t base = 0x80004000u;
    uint32_t w[] = {
        enc_bc(base, base + 12, 12, 0),
        NOP,
        enc_b(base + 8, base + 16, 0),
        NOP,
        BLR,
    };
    size_t sz; uint8_t *buf = make_dol(w, 5, &sz);
    DolImage img; dol_parse(buf, sz, &img);
    Cfg cfg;
    ASSERT_EQ_INT(0, cfg_build(&img, base, base + 20, &cfg), "build");
    /* Blocks: {0}, {4,8}, {12}, {16}. */
    ASSERT_EQ_INT(4, (int)cfg.block_count, "four blocks");
    /* B0 should be CFG_REGION_IF_ELSE with merge = B3. */
    ASSERT_TRUE(cfg.blocks[0].region_kind == CFG_REGION_IF_ELSE ||
                cfg.blocks[0].region_kind == CFG_REGION_IF,
                "B0 is a conditional region");
    ASSERT_EQ_INT(3, (int)cfg.blocks[0].if_merge, "merge is B3");
    ASSERT_EQ_INT(0, (int)cfg.is_irreducible, "reducible");
    cfg_free(&cfg);
    free(buf);
}

static void test_loop_back_edge(void) {
    /* 0: nop              ; loop header
     * 4: bc !eq -> 0      ; back-edge (cond)
     * 8: blr
     */
    uint32_t base = 0x80004000u;
    uint32_t w[] = {
        NOP,
        enc_bc(base + 4, base, 4, 2),   /* BO=4 BI=2 => bne back to 0 */
        BLR,
    };
    size_t sz; uint8_t *buf = make_dol(w, 3, &sz);
    DolImage img; dol_parse(buf, sz, &img);
    Cfg cfg;
    ASSERT_EQ_INT(0, cfg_build(&img, base, base + 12, &cfg), "build");
    /* Blocks: {0,4}, {8}. */
    ASSERT_EQ_INT(2, (int)cfg.block_count, "two blocks");
    ASSERT_TRUE(cfg.blocks[0].is_loop_header, "B0 is a loop header");
    ASSERT_EQ_INT(1, (int)cfg.blocks[0].loop_depth == 0, "B0 at depth 0");
    cfg_free(&cfg);
    free(buf);
}

int main(void) {
    printf("test_cfg\n");
    RUN_TEST(test_linear_nop_then_blr);
    RUN_TEST(test_forward_branch_splits_block);
    RUN_TEST(test_if_else_converges);
    RUN_TEST(test_loop_back_edge);
    printf("  %d run, %d passed, %d failed\n",
           tests_run, tests_passed, tests_failed);
    return tests_failed ? 1 : 0;
}
