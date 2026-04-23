/*
 * Tests for the SSA builder. We hand-assemble tiny DOL images that
 * exercise each pass (def-use, phi, const-fold, expr tree, stack
 * slots, call args).
 */
#include "cli/cfg.h"
#include "cli/ssa.h"
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

static void wbe32(uint8_t *p, uint32_t v) {
    p[0] = v >> 24; p[1] = v >> 16; p[2] = v >> 8; p[3] = v;
}

static uint8_t *make_dol(const uint32_t *ws, size_t n, size_t *sz) {
    size_t tb = n * 4u, total = 0x100 + tb;
    uint8_t *buf = calloc(total, 1);
    wbe32(buf + 0x00, 0x100);
    wbe32(buf + 0x48, 0x80004000u);
    wbe32(buf + 0x90, (uint32_t)tb);
    wbe32(buf + 0xE0, 0x80004000u);
    for (size_t i = 0; i < n; i++) wbe32(buf + 0x100 + i * 4u, ws[i]);
    *sz = total;
    return buf;
}

/* ----- encoders ----- */
static uint32_t enc_addi (unsigned rd, unsigned ra, int16_t imm) {
    return (14u << 26) | (rd << 21) | (ra << 16) | (uint16_t)imm;
}
static uint32_t enc_addis(unsigned rd, unsigned ra, int16_t imm) {
    return (15u << 26) | (rd << 21) | (ra << 16) | (uint16_t)imm;
}
static uint32_t enc_ori  (unsigned ra, unsigned rs, uint16_t imm) {
    return (24u << 26) | (rs << 21) | (ra << 16) | imm;
}
static uint32_t enc_stw  (unsigned rs, int16_t disp, unsigned ra) {
    return (36u << 26) | (rs << 21) | (ra << 16) | (uint16_t)disp;
}
static uint32_t enc_lwz  (unsigned rd, int16_t disp, unsigned ra) {
    return (32u << 26) | (rd << 21) | (ra << 16) | (uint16_t)disp;
}
static uint32_t enc_b    (uint32_t from, uint32_t to, int lk) {
    int32_t d = (int32_t)(to - from);
    return (18u << 26) | ((uint32_t)d & 0x03FFFFFCu) | (lk ? 1u : 0u);
}
static uint32_t enc_bc   (uint32_t from, uint32_t to,
                          unsigned bo, unsigned bi) {
    int32_t d = (int32_t)(to - from);
    return (16u << 26) | ((bo & 0x1Fu) << 21) | ((bi & 0x1Fu) << 16) |
           ((uint32_t)d & 0xFFFCu);
}
#define BLR 0x4E800020u

/* helper: build cfg+ssa for [0x80004000, end). */
static int build(const uint32_t *ws, size_t n, Cfg *cfg, Ssa *ssa,
                 DolImage *img, uint8_t **bufp) {
    size_t sz;
    *bufp = make_dol(ws, n, &sz);
    if (dol_parse(*bufp, sz, img) != DOL_OK) return -1;
    if (cfg_build(img, 0x80004000u, 0x80004000u + n * 4u, cfg) != 0) return -1;
    if (ssa_build(img, cfg, ssa) != 0) return -1;
    return 0;
}

/* ----- 2.1 def/use ----- */
static void test_defuse_basic(void) {
    /* addi r3, 0, 5 ; blr */
    uint32_t w[] = { enc_addi(3, 0, 5), BLR };
    Cfg cfg; Ssa ssa; DolImage img; uint8_t *buf;
    ASSERT_EQ_INT(0, build(w, 2, &cfg, &ssa, &img, &buf), "build");
    ASSERT_EQ_INT(2, (long)ssa.insn_count, "2 insns");
    ASSERT_EQ_INT(3, (long)ssa.insns[0].gpr_def, "r3 defined");
    ASSERT_TRUE((ssa.blocks[0].gpr_defs & (1ull << 3)) != 0, "block defs r3");
    ASSERT_TRUE((ssa.blocks[0].gpr_uses_before_def & (1ull << 3)) == 0,
                "r3 not live-in");
    ssa_free(&ssa); cfg_free(&cfg); free(buf);
}

/* ----- 2.3 const fold + 2.4 expr ----- */
static void test_const_fold(void) {
    /* addis r4, 0, 0x8003 ; addi r4, r4, 0x1234 ; blr
     *   => r4 = 0x80031234
     */
    uint32_t w[] = {
        enc_addis(4, 0, 0x8003),
        enc_addi (4, 4, 0x1234),
        BLR,
    };
    Cfg cfg; Ssa ssa; DolImage img; uint8_t *buf;
    ASSERT_EQ_INT(0, build(w, 3, &cfg, &ssa, &img, &buf), "build");
    ASSERT_TRUE(ssa.insns[1].const_valid, "addi folded");
    ASSERT_EQ_INT(0x80031234u, ssa.insns[1].const_value, "fold value");
    ASSERT_TRUE(ssa.insns[1].expr != NULL, "has expr");
    ssa_free(&ssa); cfg_free(&cfg); free(buf);
}

/* ----- 2.5 stack slots ----- */
static void test_stack_slots(void) {
    /* stw r0, 8(r1) ; lwz r5, 8(r1) ; stw r6, 12(r1) ; blr */
    uint32_t w[] = {
        enc_stw(0, 8, 1),
        enc_lwz(5, 8, 1),
        enc_stw(6, 12, 1),
        BLR,
    };
    Cfg cfg; Ssa ssa; DolImage img; uint8_t *buf;
    ASSERT_EQ_INT(0, build(w, 4, &cfg, &ssa, &img, &buf), "build");
    ASSERT_EQ_INT(2, (long)ssa.slot_count, "2 distinct slots");
    /* First slot: offset 8, 1 load + 1 store. */
    ASSERT_EQ_INT(8, ssa.slots[0].offset, "slot0 offset");
    ASSERT_EQ_INT(1, ssa.slots[0].load_count, "slot0 loads");
    ASSERT_EQ_INT(1, ssa.slots[0].store_count, "slot0 stores");
    ASSERT_EQ_INT(12, ssa.slots[1].offset, "slot1 offset");
    ssa_free(&ssa); cfg_free(&cfg); free(buf);
}

/* ----- 2.6 call args ----- */
static void test_call_args(void) {
    /* addi r3, 0, 1 ; addi r4, 0, 2 ; bl +16 ; addi r5, r3, 0 ; blr
     *                                          ^ consumes return value r3
     * target of bl is 0x80004010 which lives within range; CFG treats
     * in-range bl as... actually bl doesn't start a new block (calls
     * don't break blocks in the CFG builder). Good.
     */
    uint32_t base = 0x80004000u;
    uint32_t w[] = {
        enc_addi(3, 0, 1),
        enc_addi(4, 0, 2),
        enc_b(base + 8, base + 16, 1),  /* bl +8 (to offset 16) */
        enc_addi(5, 3, 0),              /* uses r3 (return val) */
        BLR,
    };
    Cfg cfg; Ssa ssa; DolImage img; uint8_t *buf;
    ASSERT_EQ_INT(0, build(w, 5, &cfg, &ssa, &img, &buf), "build");
    ASSERT_EQ_INT(1, (long)ssa.call_count, "one call site");
    const SsaCallSite *cs = &ssa.calls[0];
    ASSERT_TRUE((cs->gpr_arg_mask & 0x03) == 0x03, "r3,r4 args");
    ASSERT_EQ_INT(1, cs->returns_gpr, "r3 consumed");
    ssa_free(&ssa); cfg_free(&cfg); free(buf);
}

/* ----- 2.2 phi insertion ----- */
static void test_phi_on_join(void) {
    /* Diamond:
     *   B0: bc beq +8  -> B2  (cond-taken); ft -> B1
     *   B1: addi r3,0,1 ; b +8 (-> B3)
     *   B2: addi r3,0,2 ; (fall to B3)
     *   B3: blr
     * r3 is defined in both arms and live-in at B3 -> phi(r3)@B3.
     * But r3 is NOT used in B3, so it's not live-in there. To make it
     * interesting, let B3 also use r3 by adding ori r0,r3,0 (a no-op
     * using r3 as source).
     */
    uint32_t base = 0x80004000u;
    uint32_t w[] = {
        enc_bc(base + 0, base + 16, 12, 2),   /* beq -> pc+16 (B2) */
        enc_addi(3, 0, 1),                    /* B1 */
        enc_b(base + 8, base + 20, 0),        /* B1: b -> pc+12 (B3) */
        enc_addi(3, 0, 2),                    /* B2 (at +16 = 0x10) */
        enc_ori(0, 3, 0),                     /* B3 (at +20): uses r3 */
        BLR,
    };
    Cfg cfg; Ssa ssa; DolImage img; uint8_t *buf;
    ASSERT_EQ_INT(0, build(w, 6, &cfg, &ssa, &img, &buf), "build");
    ASSERT_TRUE(ssa.phi_count >= 1, "at least one phi placed");
    int saw_r3_phi = 0;
    for (size_t i = 0; i < ssa.phi_count; i++) {
        if (ssa.phis[i].reg_kind == 0 && ssa.phis[i].reg == 3)
            saw_r3_phi = 1;
    }
    ASSERT_TRUE(saw_r3_phi, "phi for r3 at join");
    ssa_free(&ssa); cfg_free(&cfg); free(buf);
}

int main(void) {
    printf("test_ssa\n");
    RUN_TEST(test_defuse_basic);
    RUN_TEST(test_const_fold);
    RUN_TEST(test_stack_slots);
    RUN_TEST(test_call_args);
    RUN_TEST(test_phi_on_join);
    printf("  %d run, %d passed, %d failed\n",
           tests_run, tests_passed, tests_failed);
    return tests_failed ? 1 : 0;
}
