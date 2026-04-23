/* Extended PPC decoders: rotate/mask (7.1), register-form compare
 * (7.2), multiply/divide (7.3), update-form loads/stores (7.4),
 * register-form logical ops (7.5) and Gekko paired-singles (7.6). */
#include "ppc.h"
#include "ppc_internal.h"

void ppc_decode_mform(uint32_t insn, PpcInsn *out, PpcOp op) {
    /* rlwinm / rlwimi / rlwnm share the same M-form layout. */
    out->op = op;
    out->rd = (uint8_t)ppc_bits(insn,  6, 10);   /* rS */
    out->ra = (uint8_t)ppc_bits(insn, 11, 15);   /* rA */
    out->rb = (uint8_t)ppc_bits(insn, 16, 20);   /* rB / SH */
    out->sh = (uint8_t)ppc_bits(insn, 16, 20);
    out->mb = (uint8_t)ppc_bits(insn, 21, 25);
    out->me = (uint8_t)ppc_bits(insn, 26, 30);
}

static void decode_psq_dform(uint32_t insn, PpcInsn *out, PpcOp op) {
    out->op  = op;
    out->rd  = (uint8_t)ppc_bits(insn, 6, 10);
    out->ra  = (uint8_t)ppc_bits(insn, 11, 15);
    out->sh  = (uint8_t)ppc_bits(insn, 16, 16);   /* W */
    out->mb  = (uint8_t)ppc_bits(insn, 17, 19);   /* I (GQR index) */
    out->imm = ppc_sext(ppc_bits(insn, 20, 31), 12);
}

void ppc_decode_psq(uint32_t insn, PpcInsn *out) {
    uint32_t primary = ppc_bits(insn, 0, 5);
    if (primary == 56) decode_psq_dform(insn, out, PPC_OP_PSQ_L);
    else if (primary == 57) decode_psq_dform(insn, out, PPC_OP_PSQ_LU);
    else if (primary == 60) decode_psq_dform(insn, out, PPC_OP_PSQ_ST);
    else if (primary == 61) decode_psq_dform(insn, out, PPC_OP_PSQ_STU);
}

void ppc_decode_opcode4(uint32_t insn, PpcInsn *out) {
    /* Subset of the Gekko paired-single arithmetic instructions. */
    uint32_t xo = ppc_bits(insn, 21, 30);
    uint32_t frd = ppc_bits(insn,  6, 10);
    uint32_t fra = ppc_bits(insn, 11, 15);
    uint32_t frb = ppc_bits(insn, 16, 20);
    uint32_t frc = ppc_bits(insn, 21, 25);
    switch (xo) {
        case 21:   out->op = PPC_OP_PS_ADD;
            out->rd = frd; out->ra = fra; out->rb = frb; return;
        case 20:   out->op = PPC_OP_PS_SUB;
            out->rd = frd; out->ra = fra; out->rb = frb; return;
        case 18:   out->op = PPC_OP_PS_DIV;
            out->rd = frd; out->ra = fra; out->rb = frb; return;
        case 72:   out->op = PPC_OP_PS_MR;
            out->rd = frd; out->rb = frb; return;
        case 40:   out->op = PPC_OP_PS_NEG;
            out->rd = frd; out->rb = frb; return;
        default: break;
    }
    /* Opcodes with XO split into bits 26..30. */
    uint32_t xo5 = ppc_bits(insn, 26, 30);
    switch (xo5) {
        case 25:   out->op = PPC_OP_PS_MUL;
            out->rd = frd; out->ra = fra; out->rb = frc; return;
        case 29:   out->op = PPC_OP_PS_MADD;
            out->rd = frd; out->ra = fra; out->rb = frc; return;
        case 28:   out->op = PPC_OP_PS_MSUB;
            out->rd = frd; out->ra = fra; out->rb = frc; return;
        default: break;
    }
}

typedef struct { uint32_t xo; PpcOp op; } XoMap;

/* Opcode 31 extended: arithmetic + logical + compare + shift. */
static const XoMap kOp31[] = {
    /* Compare */
    {   0, PPC_OP_CMPW },
    {  32, PPC_OP_CMPLW },
    /* Multiply / divide */
    { 235, PPC_OP_MULLW },
    {  75, PPC_OP_MULHW },
    {  11, PPC_OP_MULHWU },
    { 491, PPC_OP_DIVW },
    { 459, PPC_OP_DIVWU },
    /* Logical */
    {  28, PPC_OP_AND },
    { 444, PPC_OP_OR },
    { 316, PPC_OP_XOR },
    { 476, PPC_OP_NAND },
    { 124, PPC_OP_NOR },
    { 284, PPC_OP_EQV },
    {  60, PPC_OP_ANDC },
    { 412, PPC_OP_ORC },
    /* Shifts */
    {  24, PPC_OP_SLW },
    { 536, PPC_OP_SRW },
    { 792, PPC_OP_SRAW },
    { 824, PPC_OP_SRAWI },
    /* Add / subtract / negate */
    { 266, PPC_OP_ADD },   /* add[o][.] xo=266 */
    {  40, PPC_OP_SUBF },  /* subf */
    { 104, PPC_OP_NEG },
    /* Indexed loads / stores are useful elsewhere but not enumerated
     * here; keep the decoder focused on the requirement 7 set. */
};

int ppc_decode_op31_xform(uint32_t insn, PpcInsn *out) {
    /* Return non-zero if we matched and filled `out`. */
    uint32_t xo = ppc_bits(insn, 21, 30);
    uint32_t rd = ppc_bits(insn,  6, 10);
    uint32_t ra = ppc_bits(insn, 11, 15);
    uint32_t rb = ppc_bits(insn, 16, 20);
    for (size_t i = 0; i < sizeof kOp31 / sizeof kOp31[0]; i++) {
        if (kOp31[i].xo != xo) continue;
        PpcOp op = kOp31[i].op;
        /* For cmpw/cmplw, bits 6..8 are the crD. */
        if (op == PPC_OP_CMPW || op == PPC_OP_CMPLW) {
            if (ppc_bits(insn, 10, 10) != 0) return 0; /* L=1: 64-bit */
            out->op = op;
            out->rd = (uint8_t)ppc_bits(insn, 6, 8); /* crD */
            out->ra = (uint8_t)ra;
            out->rb = (uint8_t)rb;
            return 1;
        }
        if (op == PPC_OP_SRAWI) {
            out->op = op;
            out->rd = (uint8_t)rd;          /* rS */
            out->ra = (uint8_t)ra;          /* rA */
            out->sh = (uint8_t)rb;          /* SH in rB slot */
            return 1;
        }
        if (op == PPC_OP_NEG) {
            out->op = op;
            out->rd = (uint8_t)rd;
            out->ra = (uint8_t)ra;
            return 1;
        }
        out->op = op;
        out->rd = (uint8_t)rd;
        out->ra = (uint8_t)ra;
        out->rb = (uint8_t)rb;
        return 1;
    }
    return 0;
}
