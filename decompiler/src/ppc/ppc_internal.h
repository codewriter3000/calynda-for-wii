/* Bit-field helpers and shared declarations for the PPC decoder. */
#ifndef CALYNDA_DECOMPILER_PPC_INTERNAL_H
#define CALYNDA_DECOMPILER_PPC_INTERNAL_H

#include "ppc.h"
#include <stdint.h>

static inline uint32_t ppc_bits(uint32_t insn, uint32_t msb, uint32_t lsb) {
    uint32_t width = lsb - msb + 1;
    uint32_t shift = 31 - lsb;
    uint32_t mask  = (width == 32) ? 0xFFFFFFFFu : ((1u << width) - 1u);
    return (insn >> shift) & mask;
}

static inline int32_t ppc_sext(uint32_t v, unsigned width) {
    uint32_t sign = 1u << (width - 1);
    if (v & sign) return (int32_t)(v | ~((1u << width) - 1u));
    return (int32_t)v;
}

/* Extended decoders (see decode_ext.c). */
void ppc_decode_mform(uint32_t insn, PpcInsn *out, PpcOp op);
void ppc_decode_psq(uint32_t insn, PpcInsn *out);
void ppc_decode_opcode4(uint32_t insn, PpcInsn *out);
int  ppc_decode_op31_xform(uint32_t insn, PpcInsn *out);

#endif
