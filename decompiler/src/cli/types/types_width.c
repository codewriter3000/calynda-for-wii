/*
 * Requirement 3.1 — propagate primitive width from load/store
 * opcodes to the variable that receives (or provides) the value.
 *
 *   lbz  -> uint8   stb  -> uint8 slot
 *   lhz  -> uint16  sth  -> uint16 slot
 *   lwz  -> uint32  stw  -> uint32 slot
 *   lfs  -> float32 stfs -> float32 slot
 *   lfd  -> float64 stfd -> float64 slot
 *
 * For arithmetic producers (addi/addis/ori/mr/mulli) we default to
 * uint32 since GPRs on the Broadway are 32-bit. Float producers map
 * to float64 (FPR width); types_sign refines signedness later.
 */
#include "types_internal.h"

#include <string.h>

static TypeInfo ti_int(uint8_t w) {
    TypeInfo t = {0};
    t.kind = TYPE_INT;
    t.width = w;
    return t;
}
static TypeInfo ti_float(uint8_t w) {
    TypeInfo t = {0};
    t.kind = TYPE_FLOAT;
    t.width = w;
    return t;
}

int types_pass_width(Types *t) {
    const Ssa *ssa = t->ssa;
    for (size_t i = 0; i < ssa->insn_count; i++) {
        const SsaInsn *si = &ssa->insns[i];
        const PpcInsn *in = &si->in;
        TypeInfo info = {0};
        switch (in->op) {
            case PPC_OP_LBZ: info = ti_int(1); break;
            case PPC_OP_LHZ: info = ti_int(2); break;
            case PPC_OP_LWZ: info = ti_int(4); break;
            case PPC_OP_LFS: info = ti_float(4); break;
            case PPC_OP_LFD: info = ti_float(8); break;

            case PPC_OP_STB:
                if (si->slot_id != SSA_NO_VAL)
                    types_merge(&t->slot_type[si->slot_id], ti_int(1));
                break;
            case PPC_OP_STH:
                if (si->slot_id != SSA_NO_VAL)
                    types_merge(&t->slot_type[si->slot_id], ti_int(2));
                break;
            case PPC_OP_STW:
                if (si->slot_id != SSA_NO_VAL)
                    types_merge(&t->slot_type[si->slot_id], ti_int(4));
                break;
            case PPC_OP_STFS:
                if (si->slot_id != SSA_NO_VAL)
                    types_merge(&t->slot_type[si->slot_id], ti_float(4));
                break;
            case PPC_OP_STFD:
                if (si->slot_id != SSA_NO_VAL)
                    types_merge(&t->slot_type[si->slot_id], ti_float(8));
                break;

            case PPC_OP_ADDI: case PPC_OP_ADDIS: case PPC_OP_ORI:
            case PPC_OP_ANDI_DOT: case PPC_OP_MULLI: case PPC_OP_MR:
                info = ti_int(4);
                break;

            case PPC_OP_FMR: case PPC_OP_FNEG:
            case PPC_OP_FADD: case PPC_OP_FSUB:
            case PPC_OP_FMUL: case PPC_OP_FDIV:
                info = ti_float(8);
                break;

            default: break;
        }
        if (info.kind != TYPE_UNKNOWN)
            types_merge(&t->insn_type[i], info);

        /* Loads also refine the slot they touched. */
        if ((in->op == PPC_OP_LBZ || in->op == PPC_OP_LHZ ||
             in->op == PPC_OP_LWZ || in->op == PPC_OP_LFS ||
             in->op == PPC_OP_LFD) &&
            si->slot_id != SSA_NO_VAL) {
            types_merge(&t->slot_type[si->slot_id], info);
        }
    }
    return 0;
}
