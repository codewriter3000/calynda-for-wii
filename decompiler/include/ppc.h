/*
 * PowerPC Broadway disassembler (subset).
 *
 * Decodes 32-bit big-endian PowerPC instructions into a normalized
 * representation. Only the subset commonly seen in Wii game prologues,
 * branches, and memory access is recognized; everything else lifts to
 * PPC_OP_UNKNOWN with the raw word preserved.
 *
 * This decoder is intentionally hand-rolled and dependency-free so the
 * decompiler module stays aligned with the rest of the project.
 */
#ifndef CALYNDA_DECOMPILER_PPC_H
#define CALYNDA_DECOMPILER_PPC_H

#include <stddef.h>
#include <stdint.h>

typedef enum {
    PPC_OP_UNKNOWN = 0,

    /* I-form branches */
    PPC_OP_B,     /* b   target (AA=0, LK=0) */
    PPC_OP_BA,    /* b   target (AA=1)       */
    PPC_OP_BL,    /* bl  target (LK=1)       */
    PPC_OP_BLA,   /* bl  target (AA=1,LK=1)  */

    /* B-form conditional branch (opcode 16). Carries bo in rd, bi in
     * ra, and the resolved target. The formatter picks the
     * alias (beq/bne/blt/bge/bgt/ble/bdnz/bdz/bc). */
    PPC_OP_BC,

    /* Branch to link / count register */
    PPC_OP_BLR,   /* blr  (bclr with BO=20,BI=0) */
    PPC_OP_BCTR,  /* bctr (bcctr with BO=20,BI=0) */

    /* Integer immediate arithmetic */
    PPC_OP_ADDI,
    PPC_OP_ADDIS,
    PPC_OP_MULLI,

    /* Compare immediate */
    PPC_OP_CMPWI,  /* cmpi with L=0 */

    /* Logical immediate */
    PPC_OP_ORI,
    PPC_OP_ANDI_DOT, /* andi. */

    /* Convenience alias emitted for ori r0,r0,0 */
    PPC_OP_NOP,

    /* Load / store (D-form) */
    PPC_OP_LWZ, PPC_OP_STW,
    PPC_OP_LBZ, PPC_OP_STB,
    PPC_OP_LHZ, PPC_OP_STH,

    /* Load / store multiple word */
    PPC_OP_LMW, PPC_OP_STMW,

    /* Float load / store (D-form): frD, disp(rA) */
    PPC_OP_LFS, PPC_OP_LFD,
    PPC_OP_STFS, PPC_OP_STFD,

    /* Basic float arithmetic (A-form under opcode 63) */
    PPC_OP_FMR,   /* fmr  frD, frB           */
    PPC_OP_FNEG,  /* fneg frD, frB           */
    PPC_OP_FADD,  /* fadd frD, frA, frB      */
    PPC_OP_FSUB,  /* fsub frD, frA, frB      */
    PPC_OP_FMUL,  /* fmul frD, frA, frC      */
    PPC_OP_FDIV,  /* fdiv frD, frA, frB      */

    /* Special-purpose register moves (common cases) */
    PPC_OP_MFLR,
    PPC_OP_MTLR,
    PPC_OP_MFCTR,
    PPC_OP_MTCTR,

    /* Convenience alias for 'or rA,rS,rS' */
    PPC_OP_MR,

    /* Integer rotate-mask (M-form, opcodes 20/21/23) */
    PPC_OP_RLWINM,   /* rlwinm rA, rS, SH, MB, ME              */
    PPC_OP_RLWIMI,   /* rlwimi rA, rS, SH, MB, ME              */
    PPC_OP_RLWNM,    /* rlwnm  rA, rS, rB, MB, ME              */

    /* Compare (opcode 31 secondary) */
    PPC_OP_CMPW,     /* cmp  crD, rA, rB (L=0)                 */
    PPC_OP_CMPLW,    /* cmpl crD, rA, rB (L=0)                 */
    PPC_OP_CMPLWI,   /* cmpli crD, rA, UIMM (L=0, primary 10)  */

    /* Multiply / divide family (opcode 31) */
    PPC_OP_MULLW,    /* xo=235 */
    PPC_OP_MULHW,    /* xo=75  */
    PPC_OP_MULHWU,   /* xo=11  */
    PPC_OP_DIVW,     /* xo=491 */
    PPC_OP_DIVWU,    /* xo=459 */

    /* Load / store update-form (writes back rA = rA + disp) */
    PPC_OP_LWZU, PPC_OP_STWU,
    PPC_OP_LBZU, PPC_OP_STBU,
    PPC_OP_LHZU, PPC_OP_STHU,
    PPC_OP_LHA,  PPC_OP_LHAU,
    PPC_OP_LFSU, PPC_OP_LFDU,
    PPC_OP_STFSU, PPC_OP_STFDU,

    /* Opcode-31 logical group */
    PPC_OP_AND, PPC_OP_OR, PPC_OP_XOR,
    PPC_OP_NAND, PPC_OP_NOR, PPC_OP_EQV,
    PPC_OP_ANDC, PPC_OP_ORC,

    /* Shift register-form */
    PPC_OP_SLW, PPC_OP_SRW, PPC_OP_SRAW, PPC_OP_SRAWI,

    /* Add / subtract register-form (opcode 31) */
    PPC_OP_ADD, PPC_OP_SUBF, PPC_OP_NEG,

    /* Gekko paired-singles (Wii-specific, opcode 4 + 56 + 60) */
    PPC_OP_PS_ADD, PPC_OP_PS_SUB, PPC_OP_PS_MUL, PPC_OP_PS_DIV,
    PPC_OP_PS_MADD, PPC_OP_PS_MSUB,
    PPC_OP_PS_MR,   PPC_OP_PS_NEG,
    PPC_OP_PSQ_L,   PPC_OP_PSQ_ST,
    PPC_OP_PSQ_LU,  PPC_OP_PSQ_STU
} PpcOp;

typedef struct {
    PpcOp    op;
    uint32_t raw;       /* original 32-bit word */
    uint32_t addr;      /* virtual address supplied by caller */

    /* Generic register operands (meaning depends on op). Unused slots
     * are left zero. For mflr/mtlr/mfctr/mtctr, rd holds the GPR. */
    uint8_t  rd;
    uint8_t  ra;
    uint8_t  rb;

    /* Signed displacement / immediate for D-form and I-form */
    int32_t  imm;

    /* For branches, the resolved absolute target virtual address. */
    uint32_t target;
    uint8_t  aa;        /* absolute-address bit for I/B forms */
    uint8_t  lk;        /* link bit */

    /* Rotate-mask fields (M-form): SH in [0,31], MB/ME in [0,31].
     * Also reused for paired-singles W (quantization) / I (gqr idx). */
    uint8_t  sh;
    uint8_t  mb;
    uint8_t  me;
} PpcInsn;

/*
 * Decode a single instruction word. `vaddr` is the virtual address of
 * this word; it is used only to resolve relative branch targets.
 * Returns 0 on success (even if op == PPC_OP_UNKNOWN), -1 if `out` is
 * NULL.
 */
int ppc_decode(uint32_t word, uint32_t vaddr, PpcInsn *out);

/*
 * Format a decoded instruction as a GNU-style assembly line into `buf`.
 * Writes at most `buf_size` bytes including the terminating NUL and
 * returns the number of bytes that would have been written (snprintf
 * semantics). `buf` may be NULL only if `buf_size` is 0.
 */
int ppc_format(const PpcInsn *in, char *buf, size_t buf_size);

/* Human-readable mnemonic. Never returns NULL. */
const char *ppc_mnemonic(PpcOp op);

#endif /* CALYNDA_DECOMPILER_PPC_H */
