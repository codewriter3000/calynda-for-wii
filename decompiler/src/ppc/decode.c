#include "ppc.h"
#include "ppc_internal.h"

#include <string.h>

static uint32_t decode_spr(uint32_t insn) {
    /* SPR halves are swapped: bits 11..15 hold spr[5:9]. */
    uint32_t field = ppc_bits(insn, 11, 20);
    return ((field & 0x1F) << 5) | ((field >> 5) & 0x1F);
}

static void decode_iform_branch(uint32_t insn, uint32_t vaddr, PpcInsn *out) {
    uint32_t li_raw = ppc_bits(insn, 6, 29);
    int32_t  disp   = ppc_sext(li_raw << 2, 26);
    out->aa = (uint8_t)ppc_bits(insn, 30, 30);
    out->lk = (uint8_t)ppc_bits(insn, 31, 31);
    out->target = out->aa ? (uint32_t)disp : (vaddr + (uint32_t)disp);
    if (out->aa && out->lk)      out->op = PPC_OP_BLA;
    else if (out->aa)            out->op = PPC_OP_BA;
    else if (out->lk)            out->op = PPC_OP_BL;
    else                         out->op = PPC_OP_B;
}

static void decode_bform_branch(uint32_t insn, uint32_t vaddr, PpcInsn *out) {
    uint32_t bd_raw = ppc_bits(insn, 16, 29);
    int32_t  disp   = ppc_sext(bd_raw << 2, 16);
    out->op     = PPC_OP_BC;
    out->rd     = (uint8_t)ppc_bits(insn,  6, 10);  /* BO */
    out->ra     = (uint8_t)ppc_bits(insn, 11, 15);  /* BI */
    out->aa     = (uint8_t)ppc_bits(insn, 30, 30);
    out->lk     = (uint8_t)ppc_bits(insn, 31, 31);
    out->target = out->aa ? (uint32_t)disp : (vaddr + (uint32_t)disp);
}

static void decode_opcode19(uint32_t insn, PpcInsn *out) {
    uint32_t xo = ppc_bits(insn, 21, 30);
    uint32_t bo = ppc_bits(insn, 6, 10);
    uint32_t bi = ppc_bits(insn, 11, 15);
    out->lk = (uint8_t)ppc_bits(insn, 31, 31);
    if (xo == 16 && bo == 20 && bi == 0 && out->lk == 0)
        out->op = PPC_OP_BLR;
    else if (xo == 528 && bo == 20 && bi == 0 && out->lk == 0)
        out->op = PPC_OP_BCTR;
}

static void decode_dform_arith(uint32_t insn, PpcInsn *out, PpcOp op) {
    out->op  = op;
    out->rd  = (uint8_t)ppc_bits(insn, 6, 10);
    out->ra  = (uint8_t)ppc_bits(insn, 11, 15);
    out->imm = ppc_sext(ppc_bits(insn, 16, 31), 16);
}

static void decode_dform_mem(uint32_t insn, PpcInsn *out, PpcOp op) {
    out->op  = op;
    out->rd  = (uint8_t)ppc_bits(insn, 6, 10);
    out->ra  = (uint8_t)ppc_bits(insn, 11, 15);
    out->imm = ppc_sext(ppc_bits(insn, 16, 31), 16);
}

static void decode_ori(uint32_t insn, PpcInsn *out) {
    out->rd  = (uint8_t)ppc_bits(insn,  6, 10);
    out->ra  = (uint8_t)ppc_bits(insn, 11, 15);
    out->imm = (int32_t)ppc_bits(insn, 16, 31);
    out->op  = (out->rd == 0 && out->ra == 0 && out->imm == 0)
               ? PPC_OP_NOP : PPC_OP_ORI;
}

static void decode_andi_dot(uint32_t insn, PpcInsn *out) {
    out->op  = PPC_OP_ANDI_DOT;
    out->rd  = (uint8_t)ppc_bits(insn,  6, 10);
    out->ra  = (uint8_t)ppc_bits(insn, 11, 15);
    out->imm = (int32_t)ppc_bits(insn, 16, 31);
}

static void decode_cmpi(uint32_t insn, PpcInsn *out) {
    if (ppc_bits(insn, 10, 10) != 0) return; /* cmpdi: unsupported */
    out->op  = PPC_OP_CMPWI;
    out->rd  = (uint8_t)ppc_bits(insn, 6, 8);
    out->ra  = (uint8_t)ppc_bits(insn, 11, 15);
    out->imm = ppc_sext(ppc_bits(insn, 16, 31), 16);
}

static void decode_opcode31(uint32_t insn, PpcInsn *out) {
    uint32_t xo = ppc_bits(insn, 21, 30);
    if (xo == 339) {                     /* mfspr */
        uint32_t spr = decode_spr(insn);
        out->rd = (uint8_t)ppc_bits(insn, 6, 10);
        if (spr == 8)      out->op = PPC_OP_MFLR;
        else if (spr == 9) out->op = PPC_OP_MFCTR;
        return;
    }
    if (xo == 467) {                     /* mtspr */
        uint32_t spr = decode_spr(insn);
        out->rd = (uint8_t)ppc_bits(insn, 6, 10);
        if (spr == 8)      out->op = PPC_OP_MTLR;
        else if (spr == 9) out->op = PPC_OP_MTCTR;
        return;
    }
    if (xo == 444) {                     /* or — mr alias when rS == rB */
        uint32_t rs = ppc_bits(insn, 6, 10);
        uint32_t ra = ppc_bits(insn, 11, 15);
        uint32_t rb = ppc_bits(insn, 16, 20);
        uint32_t rc = ppc_bits(insn, 31, 31);
        if (rc == 0 && rs == rb) {
            out->op = PPC_OP_MR;
            out->rd = (uint8_t)ra;
            out->ra = (uint8_t)rs;
            return;
        }
        /* Fall through to the generic `or rA, rS, rB` below. */
    }
    /* Shared table for xform arithmetic / logical / compare ops. */
    ppc_decode_op31_xform(insn, out);
}

static void decode_opcode63(uint32_t insn, PpcInsn *out) {
    /* Primary 63: float arithmetic (A-form). Ignore Rc bit. */
    uint32_t xo = ppc_bits(insn, 21, 30);
    uint32_t frd = ppc_bits(insn,  6, 10);
    uint32_t fra = ppc_bits(insn, 11, 15);
    uint32_t frb = ppc_bits(insn, 16, 20);
    uint32_t frc = ppc_bits(insn, 21, 25);
    switch (xo) {
        case 72:  /* fmr  frD, frB  */
            out->op = PPC_OP_FMR;
            out->rd = (uint8_t)frd; out->rb = (uint8_t)frb;
            break;
        case 40:  /* fneg frD, frB  */
            out->op = PPC_OP_FNEG;
            out->rd = (uint8_t)frd; out->rb = (uint8_t)frb;
            break;
        case 21:  /* fadd frD, frA, frB */
            out->op = PPC_OP_FADD;
            out->rd = (uint8_t)frd; out->ra = (uint8_t)fra; out->rb = (uint8_t)frb;
            break;
        case 20:  /* fsub frD, frA, frB */
            out->op = PPC_OP_FSUB;
            out->rd = (uint8_t)frd; out->ra = (uint8_t)fra; out->rb = (uint8_t)frb;
            break;
        case 25:  /* fmul frD, frA, frC */
            out->op = PPC_OP_FMUL;
            out->rd = (uint8_t)frd; out->ra = (uint8_t)fra; out->rb = (uint8_t)frc;
            break;
        case 18:  /* fdiv frD, frA, frB */
            out->op = PPC_OP_FDIV;
            out->rd = (uint8_t)frd; out->ra = (uint8_t)fra; out->rb = (uint8_t)frb;
            break;
        default: break;
    }
}

int ppc_decode(uint32_t word, uint32_t vaddr, PpcInsn *out) {
    if (!out) return -1;
    memset(out, 0, sizeof(*out));
    out->raw  = word;
    out->addr = vaddr;
    out->op   = PPC_OP_UNKNOWN;

    uint32_t primary = ppc_bits(word, 0, 5);
    switch (primary) {
        case 4:  ppc_decode_opcode4(word, out);               break;
        case 7:  decode_dform_arith(word, out, PPC_OP_MULLI); break;
        case 10: /* cmpli — cmplwi when L=0. */
            if (ppc_bits(word, 10, 10) == 0) {
                out->op  = PPC_OP_CMPLWI;
                out->rd  = (uint8_t)ppc_bits(word, 6, 8);
                out->ra  = (uint8_t)ppc_bits(word, 11, 15);
                out->imm = (int32_t)ppc_bits(word, 16, 31);
            }
            break;
        case 11: decode_cmpi(word, out); break;
        case 14: decode_dform_arith(word, out, PPC_OP_ADDI);  break;
        case 15: decode_dform_arith(word, out, PPC_OP_ADDIS); break;
        case 16: decode_bform_branch(word, vaddr, out);       break;
        case 18: decode_iform_branch(word, vaddr, out);       break;
        case 19: decode_opcode19(word, out);                  break;
        case 20: ppc_decode_mform(word, out, PPC_OP_RLWIMI);  break;
        case 21: ppc_decode_mform(word, out, PPC_OP_RLWINM);  break;
        case 23: ppc_decode_mform(word, out, PPC_OP_RLWNM);   break;
        case 24: decode_ori(word, out);                       break;
        case 28: decode_andi_dot(word, out);                  break;
        case 31: decode_opcode31(word, out);                  break;
        case 32: decode_dform_mem(word, out, PPC_OP_LWZ);     break;
        case 33: decode_dform_mem(word, out, PPC_OP_LWZU);    break;
        case 34: decode_dform_mem(word, out, PPC_OP_LBZ);     break;
        case 35: decode_dform_mem(word, out, PPC_OP_LBZU);    break;
        case 36: decode_dform_mem(word, out, PPC_OP_STW);     break;
        case 37: decode_dform_mem(word, out, PPC_OP_STWU);    break;
        case 38: decode_dform_mem(word, out, PPC_OP_STB);     break;
        case 39: decode_dform_mem(word, out, PPC_OP_STBU);    break;
        case 40: decode_dform_mem(word, out, PPC_OP_LHZ);     break;
        case 41: decode_dform_mem(word, out, PPC_OP_LHZU);    break;
        case 42: decode_dform_mem(word, out, PPC_OP_LHA);     break;
        case 43: decode_dform_mem(word, out, PPC_OP_LHAU);    break;
        case 44: decode_dform_mem(word, out, PPC_OP_STH);     break;
        case 45: decode_dform_mem(word, out, PPC_OP_STHU);    break;
        case 46: decode_dform_mem(word, out, PPC_OP_LMW);     break;
        case 47: decode_dform_mem(word, out, PPC_OP_STMW);    break;
        case 48: decode_dform_mem(word, out, PPC_OP_LFS);     break;
        case 49: decode_dform_mem(word, out, PPC_OP_LFSU);    break;
        case 50: decode_dform_mem(word, out, PPC_OP_LFD);     break;
        case 51: decode_dform_mem(word, out, PPC_OP_LFDU);    break;
        case 52: decode_dform_mem(word, out, PPC_OP_STFS);    break;
        case 53: decode_dform_mem(word, out, PPC_OP_STFSU);   break;
        case 54: decode_dform_mem(word, out, PPC_OP_STFD);    break;
        case 55: decode_dform_mem(word, out, PPC_OP_STFDU);   break;
        case 56: ppc_decode_psq(word, out);                   break;
        case 57: ppc_decode_psq(word, out);                   break;
        case 60: ppc_decode_psq(word, out);                   break;
        case 61: ppc_decode_psq(word, out);                   break;
        case 63: decode_opcode63(word, out);                  break;
        default: break;
    }
    return 0;
}
