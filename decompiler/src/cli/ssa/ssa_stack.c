/*
 * Requirement 2.5 — recognize stack slot references of the form
 *     stw  rX,  N(r1)
 *     lwz  rX,  N(r1)
 * plus their byte/half/multiple/float variants, and assign each
 * distinct (offset, width, is_float) triple a stable local name.
 *
 * This pass also patches every matching SsaInsn with a slot_id so
 * later stages can rewrite the operand from `N(r1)` to `local_<n>`.
 */
#include "ssa_internal.h"

#include <stdlib.h>
#include <string.h>

static uint8_t width_of(PpcOp op) {
    switch (op) {
        case PPC_OP_LBZ: case PPC_OP_STB: return 1;
        case PPC_OP_LHZ: case PPC_OP_STH: return 2;
        case PPC_OP_LWZ: case PPC_OP_STW:
        case PPC_OP_LFS: case PPC_OP_STFS: return 4;
        case PPC_OP_LFD: case PPC_OP_STFD: return 8;
        case PPC_OP_LMW: case PPC_OP_STMW: return 4;
        default: return 0;
    }
}

static int is_float_op(PpcOp op) {
    return op == PPC_OP_LFS || op == PPC_OP_LFD ||
           op == PPC_OP_STFS || op == PPC_OP_STFD;
}

static int is_store_op(PpcOp op) {
    return op == PPC_OP_STW || op == PPC_OP_STB || op == PPC_OP_STH ||
           op == PPC_OP_STMW || op == PPC_OP_STFS || op == PPC_OP_STFD;
}

static int find_or_add_slot(Ssa *ssa, int32_t off, uint8_t w, uint8_t fp,
                            uint32_t *out_id) {
    for (size_t i = 0; i < ssa->slot_count; i++) {
        SsaStackSlot *s = &ssa->slots[i];
        if (s->offset == off && s->width == w && s->is_float == fp) {
            *out_id = (uint32_t)i;
            return 0;
        }
    }
    size_t n = ssa->slot_count + 1;
    SsaStackSlot *np = (SsaStackSlot *)realloc(ssa->slots,
                                               n * sizeof(SsaStackSlot));
    if (!np) return -1;
    ssa->slots = np;
    SsaStackSlot *s = &ssa->slots[ssa->slot_count];
    memset(s, 0, sizeof(*s));
    s->offset   = off;
    s->width    = w;
    s->is_float = fp;
    const char *name;
    if (off >= 0) name = ssa_internf(ssa, "local_%d", off);
    else          name = ssa_internf(ssa, "local_m%d", -off);
    if (!name) return -1;
    s->name = name;
    *out_id = (uint32_t)ssa->slot_count++;
    return 0;
}

int ssa_map_stack_slots(Ssa *ssa) {
    for (size_t i = 0; i < ssa->insn_count; i++) {
        SsaInsn *si = &ssa->insns[i];
        uint8_t w = width_of(si->in.op);
        if (w == 0) continue;
        if (si->in.ra != 1) continue;  /* not r1-relative */

        uint32_t id;
        if (find_or_add_slot(ssa, si->in.imm, w,
                             (uint8_t)is_float_op(si->in.op), &id) != 0)
            return -1;
        si->slot_id = id;
        if (is_store_op(si->in.op)) ssa->slots[id].store_count++;
        else                        ssa->slots[id].load_count++;
    }
    return 0;
}
