#include "ppc.h"

#include <stdio.h>

/* Translate a (bo, bi) pair into a convenient mnemonic. Returns NULL
 * when no alias applies and the caller should emit "bc bo, bi, target".
 * BI selects a CR bit: (cr * 4) + bit, where bit 0=LT, 1=GT, 2=EQ, 3=SO.
 */
static const char *bc_alias(uint32_t bo, uint32_t bi, int *uses_target_only) {
    *uses_target_only = 1;
    uint32_t bit = bi & 3;
    if (bo == 16)  return "bdnz";     /* decrement CTR, branch if != 0 */
    if (bo == 18)  return "bdz";      /* decrement CTR, branch if == 0 */
    if (bo == 20) { *uses_target_only = 1; return "b"; } /* unconditional */
    if (bo == 12) {                   /* branch if CR bit set */
        switch (bit) {
            case 0: return "blt";
            case 1: return "bgt";
            case 2: return "beq";
            case 3: return "bso";
        }
    }
    if (bo == 4) {                    /* branch if CR bit clear */
        switch (bit) {
            case 0: return "bge";
            case 1: return "ble";
            case 2: return "bne";
            case 3: return "bns";
        }
    }
    *uses_target_only = 0;
    return NULL;
}

static int format_bc(const PpcInsn *in, char *buf, size_t buf_size) {
    int target_only = 0;
    const char *alias = bc_alias(in->rd, in->ra, &target_only);
    uint32_t cr = (in->ra >> 2) & 7;
    const char *lk = in->lk ? "l" : "";
    if (alias) {
        if (target_only || cr == 0) {
            return snprintf(buf, buf_size, "%s%s 0x%08x",
                            alias, lk, in->target);
        }
        return snprintf(buf, buf_size, "%s%s cr%u, 0x%08x",
                        alias, lk, cr, in->target);
    }
    return snprintf(buf, buf_size, "bc%s %u, %u, 0x%08x",
                    lk, in->rd, in->ra, in->target);
}

int ppc_format(const PpcInsn *in, char *buf, size_t buf_size) {
    if (!in) return -1;
    const char *m = ppc_mnemonic(in->op);
    switch (in->op) {
        case PPC_OP_UNKNOWN:
            return snprintf(buf, buf_size, ".long 0x%08x", in->raw);
        case PPC_OP_B: case PPC_OP_BL: case PPC_OP_BA: case PPC_OP_BLA:
            return snprintf(buf, buf_size, "%s 0x%08x", m, in->target);
        case PPC_OP_BC:
            return format_bc(in, buf, buf_size);
        case PPC_OP_BLR: case PPC_OP_BCTR: case PPC_OP_NOP:
            return snprintf(buf, buf_size, "%s", m);
        case PPC_OP_ADDI: case PPC_OP_ADDIS: case PPC_OP_MULLI:
            return snprintf(buf, buf_size, "%s r%u, r%u, %d",
                            m, in->rd, in->ra, in->imm);
        case PPC_OP_CMPWI:
            return snprintf(buf, buf_size, "%s cr%u, r%u, %d",
                            m, in->rd, in->ra, in->imm);
        case PPC_OP_ORI: case PPC_OP_ANDI_DOT:
            return snprintf(buf, buf_size, "%s r%u, r%u, %d",
                            m, in->ra, in->rd, in->imm);
        case PPC_OP_LWZ: case PPC_OP_STW:
        case PPC_OP_LBZ: case PPC_OP_STB:
        case PPC_OP_LHZ: case PPC_OP_STH:
        case PPC_OP_LMW: case PPC_OP_STMW:
            return snprintf(buf, buf_size, "%s r%u, %d(r%u)",
                            m, in->rd, in->imm, in->ra);
        case PPC_OP_LFS: case PPC_OP_LFD:
        case PPC_OP_STFS: case PPC_OP_STFD:
            return snprintf(buf, buf_size, "%s f%u, %d(r%u)",
                            m, in->rd, in->imm, in->ra);
        case PPC_OP_FMR: case PPC_OP_FNEG:
            return snprintf(buf, buf_size, "%s f%u, f%u",
                            m, in->rd, in->rb);
        case PPC_OP_FADD: case PPC_OP_FSUB:
        case PPC_OP_FMUL: case PPC_OP_FDIV:
            return snprintf(buf, buf_size, "%s f%u, f%u, f%u",
                            m, in->rd, in->ra, in->rb);
        case PPC_OP_MFLR: case PPC_OP_MFCTR:
        case PPC_OP_MTLR: case PPC_OP_MTCTR:
            return snprintf(buf, buf_size, "%s r%u", m, in->rd);
        case PPC_OP_MR:
            return snprintf(buf, buf_size, "%s r%u, r%u",
                            m, in->rd, in->ra);
        case PPC_OP_RLWINM: case PPC_OP_RLWIMI:
            return snprintf(buf, buf_size,
                            "%s r%u, r%u, %u, %u, %u",
                            m, in->ra, in->rd,
                            in->sh, in->mb, in->me);
        case PPC_OP_RLWNM:
            return snprintf(buf, buf_size,
                            "%s r%u, r%u, r%u, %u, %u",
                            m, in->ra, in->rd, in->rb,
                            in->mb, in->me);
        case PPC_OP_CMPW: case PPC_OP_CMPLW:
            return snprintf(buf, buf_size, "%s cr%u, r%u, r%u",
                            m, in->rd, in->ra, in->rb);
        case PPC_OP_CMPLWI:
            return snprintf(buf, buf_size, "%s cr%u, r%u, %d",
                            m, in->rd, in->ra, in->imm);
        case PPC_OP_MULLW: case PPC_OP_MULHW: case PPC_OP_MULHWU:
        case PPC_OP_DIVW:  case PPC_OP_DIVWU:
        case PPC_OP_ADD:   case PPC_OP_SUBF:
        case PPC_OP_AND:   case PPC_OP_OR:   case PPC_OP_XOR:
        case PPC_OP_NAND:  case PPC_OP_NOR:  case PPC_OP_EQV:
        case PPC_OP_ANDC:  case PPC_OP_ORC:
        case PPC_OP_SLW:   case PPC_OP_SRW:  case PPC_OP_SRAW:
            /* X-form: for logical ops, operand order is rA, rS, rB. */
            if (in->op == PPC_OP_AND || in->op == PPC_OP_OR ||
                in->op == PPC_OP_XOR || in->op == PPC_OP_NAND ||
                in->op == PPC_OP_NOR || in->op == PPC_OP_EQV ||
                in->op == PPC_OP_ANDC || in->op == PPC_OP_ORC ||
                in->op == PPC_OP_SLW || in->op == PPC_OP_SRW ||
                in->op == PPC_OP_SRAW) {
                return snprintf(buf, buf_size, "%s r%u, r%u, r%u",
                                m, in->ra, in->rd, in->rb);
            }
            return snprintf(buf, buf_size, "%s r%u, r%u, r%u",
                            m, in->rd, in->ra, in->rb);
        case PPC_OP_SRAWI:
            return snprintf(buf, buf_size, "%s r%u, r%u, %u",
                            m, in->ra, in->rd, in->sh);
        case PPC_OP_NEG:
            return snprintf(buf, buf_size, "%s r%u, r%u",
                            m, in->rd, in->ra);
        case PPC_OP_LWZU: case PPC_OP_STWU:
        case PPC_OP_LBZU: case PPC_OP_STBU:
        case PPC_OP_LHZU: case PPC_OP_STHU:
        case PPC_OP_LHA:  case PPC_OP_LHAU:
            return snprintf(buf, buf_size, "%s r%u, %d(r%u)",
                            m, in->rd, in->imm, in->ra);
        case PPC_OP_LFSU: case PPC_OP_LFDU:
        case PPC_OP_STFSU: case PPC_OP_STFDU:
            return snprintf(buf, buf_size, "%s f%u, %d(r%u)",
                            m, in->rd, in->imm, in->ra);
        case PPC_OP_PS_ADD: case PPC_OP_PS_SUB:
        case PPC_OP_PS_MUL: case PPC_OP_PS_DIV:
            return snprintf(buf, buf_size, "%s f%u, f%u, f%u",
                            m, in->rd, in->ra, in->rb);
        case PPC_OP_PS_MADD: case PPC_OP_PS_MSUB:
            return snprintf(buf, buf_size, "%s f%u, f%u, f%u",
                            m, in->rd, in->ra, in->rb);
        case PPC_OP_PS_MR: case PPC_OP_PS_NEG:
            return snprintf(buf, buf_size, "%s f%u, f%u",
                            m, in->rd, in->rb);
        case PPC_OP_PSQ_L: case PPC_OP_PSQ_LU:
        case PPC_OP_PSQ_ST: case PPC_OP_PSQ_STU:
            return snprintf(buf, buf_size, "%s f%u, %d(r%u), %u, %u",
                            m, in->rd, in->imm, in->ra, in->sh, in->mb);
    }
    return snprintf(buf, buf_size, "?");
}
