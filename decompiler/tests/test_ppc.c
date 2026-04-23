#include "ppc.h"

#include <stdio.h>
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

static void decode_assert(uint32_t word, uint32_t vaddr,
                          PpcOp expected_op, const char *expected_fmt) {
    PpcInsn in;
    ASSERT_EQ_INT(0, ppc_decode(word, vaddr, &in), "decode ok");
    ASSERT_EQ_INT(expected_op, in.op, "op");
    char buf[64];
    ppc_format(&in, buf, sizeof(buf));
    ASSERT_EQ_STR(expected_fmt, buf, "format");
}

static void test_common_prologue_ops(void) {
    /* mflr r0            = 0x7C0802A6 */
    decode_assert(0x7C0802A6u, 0, PPC_OP_MFLR, "mflr r0");
    /* mtlr r0            = 0x7C0803A6 */
    decode_assert(0x7C0803A6u, 0, PPC_OP_MTLR, "mtlr r0");
    /* stw  r0, 4(r1)     = 0x90010004 */
    decode_assert(0x90010004u, 0, PPC_OP_STW,  "stw r0, 4(r1)");
    /* lwz  r0, 4(r1)     = 0x80010004 */
    decode_assert(0x80010004u, 0, PPC_OP_LWZ,  "lwz r0, 4(r1)");
    /* addi r1, r1, -16   = 0x3821FFF0 */
    decode_assert(0x3821FFF0u, 0, PPC_OP_ADDI, "addi r1, r1, -16");
    /* nop  = ori r0,r0,0 = 0x60000000 */
    decode_assert(0x60000000u, 0, PPC_OP_NOP,  "nop");
    /* blr                = 0x4E800020 */
    decode_assert(0x4E800020u, 0, PPC_OP_BLR,  "blr");
    /* bctr               = 0x4E800420 */
    decode_assert(0x4E800420u, 0, PPC_OP_BCTR, "bctr");
}

static void test_branches_resolve_target(void) {
    /* b +8 from 0x80003100 -> 0x80003108, encoding 0x48000008 */
    decode_assert(0x48000008u, 0x80003100u, PPC_OP_B,  "b 0x80003108");
    /* bl +12 from 0x80003100 -> 0x8000310C, encoding 0x4800000D */
    decode_assert(0x4800000Du, 0x80003100u, PPC_OP_BL, "bl 0x8000310c");
    /* Negative displacement: b -4 from 0x80003104 -> 0x80003100.
     * LI = -4 = 0xFFFFFFFC truncated to 24 bits after shift: I-form disp
     * bits are [6..29] = (LI>>2) & 0xFFFFFF; LI>>2 == 0xFFFFFFFF.
     * insn = (18<<26) | ((LI & 0x03FFFFFC)) = 0x48000000 | 0x03FFFFFC
     *      = 0x4BFFFFFC
     */
    decode_assert(0x4BFFFFFCu, 0x80003104u, PPC_OP_B, "b 0x80003100");
}

static void test_arith_and_logic(void) {
    /* addis r3, r0, -32768 (0x8000 signed) = 0x3C608000 */
    decode_assert(0x3C608000u, 0, PPC_OP_ADDIS, "addis r3, r0, -32768");
    /* cmpwi cr0, r3, 0 = 0x2C030000 */
    decode_assert(0x2C030000u, 0, PPC_OP_CMPWI, "cmpwi cr0, r3, 0");
    /* mr r3, r4  = or r3,r4,r4 = 0x7C832378 */
    decode_assert(0x7C832378u, 0, PPC_OP_MR, "mr r3, r4");
}

static void test_unknown_falls_through(void) {
    PpcInsn in;
    /* Primary opcode 0 is invalid in the subset we support. */
    ASSERT_EQ_INT(0, ppc_decode(0x00000000u, 0, &in), "decode ok");
    ASSERT_EQ_INT(PPC_OP_UNKNOWN, in.op, "unknown op");
    char buf[64];
    ppc_format(&in, buf, sizeof(buf));
    ASSERT_EQ_STR(".long 0x00000000", buf, "unknown format");
}

/* 7.1 rotate / mask */
static void test_rotate_mask(void) {
    /* rlwinm r3, r4, 16, 16, 31 : primary=21 rS=4 rA=3 SH=16 MB=16 ME=31 Rc=0
     * (21<<26)|(4<<21)|(3<<16)|(16<<11)|(16<<6)|(31<<1)|0
     *  = 0x54000000|0x00800000|0x00030000|0x00008000|0x00000400|0x0000003E
     *  = 0x5483843E
     */
    decode_assert(0x5483843Eu, 0, PPC_OP_RLWINM,
                  "rlwinm r3, r4, 16, 16, 31");
    /* rlwimi r5, r6, 8, 0, 7 : (20<<26)|(6<<21)|(5<<16)|(8<<11)|(0<<6)|(7<<1)
     *  = 0x50000000|0x00C00000|0x00050000|0x00004000|0x00000000|0x0000000E
     *  = 0x50C5400E
     */
    decode_assert(0x50C5400Eu, 0, PPC_OP_RLWIMI,
                  "rlwimi r5, r6, 8, 0, 7");
}

/* 7.2 register-form compare */
static void test_compare_reg(void) {
    /* cmpw cr0, r3, r4 : (31<<26)|(0<<23)|(0<<22)|(0<<21)|(3<<16)|(4<<11)|
     *                   (0<<10)|(0<<1)|0
     *  = 0x7C000000|0|0|0|0x00030000|0x00002000|0 = 0x7C032000
     */
    decode_assert(0x7C032000u, 0, PPC_OP_CMPW, "cmpw cr0, r3, r4");
    /* cmplw cr1, r5, r6 : xo=32 : 0x7C000040 with crD=1 in bits6..8
     *  (31<<26)|(1<<23)|(5<<16)|(6<<11)|(32<<1)
     *  = 0x7C000000|0x00800000|0x00050000|0x00003000|0x00000040
     *  = 0x7C853040
     */
    decode_assert(0x7C853040u, 0, PPC_OP_CMPLW, "cmplw cr1, r5, r6");
    /* cmplwi cr0, r3, 0x1234 : (10<<26)|(3<<16)|0x1234 = 0x28031234 */
    decode_assert(0x28031234u, 0, PPC_OP_CMPLWI,
                  "cmplwi cr0, r3, 4660");
}

/* 7.3 multiply / divide */
static void test_muldiv(void) {
    /* mullw r3, r4, r5 : xo=235 Rc=0
     *  (31<<26)|(3<<21)|(4<<16)|(5<<11)|(235<<1)
     *  = 0x7C000000|0x00600000|0x00040000|0x00002800|0x000001D6
     *  = 0x7C6429D6
     */
    decode_assert(0x7C6429D6u, 0, PPC_OP_MULLW, "mullw r3, r4, r5");
    /* divwu r6, r7, r8 : xo=459
     *  (31<<26)|(6<<21)|(7<<16)|(8<<11)|(459<<1)
     *  = 0x7C000000|0x00C00000|0x00070000|0x00004000|0x00000396
     *  = 0x7CC74396
     */
    decode_assert(0x7CC74396u, 0, PPC_OP_DIVWU, "divwu r6, r7, r8");
}

/* 7.4 update-form loads/stores */
static void test_update_form_mem(void) {
    /* lwzu r3, 8(r4) : primary=33 (33<<26)|(3<<21)|(4<<16)|8 = 0x84640008 */
    decode_assert(0x84640008u, 0, PPC_OP_LWZU, "lwzu r3, 8(r4)");
    /* stwu r1, -32(r1) : primary=37 imm=0xFFE0
     *  = 0x94000000|0x00200000|0x00010000|0xFFE0 = 0x9421FFE0
     */
    decode_assert(0x9421FFE0u, 0, PPC_OP_STWU, "stwu r1, -32(r1)");
    /* lha r3, 4(r4) : primary=42 : 0xA8640004 */
    decode_assert(0xA8640004u, 0, PPC_OP_LHA, "lha r3, 4(r4)");
}

/* 7.5 opcode-31 logical */
static void test_logical_reg(void) {
    /* and r3, r4, r5 : xo=28
     *  (31<<26)|(4<<21)|(3<<16)|(5<<11)|(28<<1)
     *  = 0x7C000000|0x00800000|0x00030000|0x00002800|0x00000038
     *  = 0x7C832838
     */
    decode_assert(0x7C832838u, 0, PPC_OP_AND, "and r3, r4, r5");
    /* xor r6, r7, r8 : xo=316
     *  (31<<26)|(7<<21)|(6<<16)|(8<<11)|(316<<1)
     *  = 0x7C000000|0x00E00000|0x00060000|0x00004000|0x00000278
     *  = 0x7CE64278
     */
    decode_assert(0x7CE64278u, 0, PPC_OP_XOR, "xor r6, r7, r8");
    /* srawi r3, r4, 5 : xo=824 SH=5 in bits 16..20
     *  (31<<26)|(4<<21)|(3<<16)|(5<<11)|(824<<1)
     *  = 0x7C000000|0x00800000|0x00030000|0x00002800|0x00000670
     *  = 0x7C832E70
     */
    decode_assert(0x7C832E70u, 0, PPC_OP_SRAWI, "srawi r3, r4, 5");
    /* add r3, r4, r5 : xo=266
     *  (31<<26)|(3<<21)|(4<<16)|(5<<11)|(266<<1)
     *  = 0x7C000000|0x00600000|0x00040000|0x00002800|0x00000214
     *  = 0x7C642A14
     */
    decode_assert(0x7C642A14u, 0, PPC_OP_ADD, "add r3, r4, r5");
}

/* 7.6 paired-singles */
static void test_paired_singles(void) {
    /* ps_add fp1, fp2, fp3 : primary=4 xo=21
     *  (4<<26)|(1<<21)|(2<<16)|(3<<11)|(21<<1)
     *  = 0x10000000|0x00200000|0x00020000|0x00001800|0x0000002A
     *  = 0x1022182A
     */
    decode_assert(0x1022182Au, 0, PPC_OP_PS_ADD,
                  "ps_add f1, f2, f3");
    /* psq_l fp1, 16(r2), 0, 0 : primary=56
     *  (56<<26)|(1<<21)|(2<<16)|(0<<15)|(0<<12)|16
     *  = 0xE0000000|0x00200000|0x00020000|16 = 0xE0220010
     */
    decode_assert(0xE0220010u, 0, PPC_OP_PSQ_L,
                  "psq_l f1, 16(r2), 0, 0");
}

int main(void) {
    printf("test_ppc\n");
    RUN_TEST(test_common_prologue_ops);
    RUN_TEST(test_branches_resolve_target);
    RUN_TEST(test_arith_and_logic);
    RUN_TEST(test_rotate_mask);
    RUN_TEST(test_compare_reg);
    RUN_TEST(test_muldiv);
    RUN_TEST(test_update_form_mem);
    RUN_TEST(test_logical_reg);
    RUN_TEST(test_paired_singles);
    RUN_TEST(test_unknown_falls_through);
    printf("  %d run, %d passed, %d failed\n",
           tests_run, tests_passed, tests_failed);
    return tests_failed == 0 ? 0 : 1;
}
