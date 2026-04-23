/*
 * Requirement 3.5 — array/pointer disambiguation.
 *
 * Inside any CFG loop (a block whose loop_depth > 0 or is a loop
 * header), look for the classic array-indexing stride pattern:
 *
 *   addi rY, rY, K    ;  K is a power-of-two or small positive constant
 *   lwz  rX, 0(rY)    ;  (or byte/half/float load/store)
 *
 * Also match the equivalent form where the same base register rY is
 * used by multiple loads at different constant offsets within one
 * block — another signal that rY points at an array of records.
 *
 * For each detected array-indexing site we tag the defining
 * instruction of the base register (as best we can: the nearest
 * preceding def of rY in the same block) with is_array + stride.
 */
#include "types_internal.h"

#include <stdlib.h>
#include <string.h>

static uint8_t elem_width(PpcOp op) {
    switch (op) {
        case PPC_OP_LBZ: case PPC_OP_STB: return 1;
        case PPC_OP_LHZ: case PPC_OP_STH: return 2;
        case PPC_OP_LWZ: case PPC_OP_STW: return 4;
        case PPC_OP_LFS: case PPC_OP_STFS: return 4;
        case PPC_OP_LFD: case PPC_OP_STFD: return 8;
        default: return 0;
    }
}

/* Mark the most recent in-block def of `reg` as an array. */
static void mark_base_as_array(Types *t, const SsaBlockInfo *b,
                               size_t from, uint8_t reg,
                               uint16_t stride) {
    const Ssa *ssa = t->ssa;
    for (size_t k = from; k-- > b->first_insn;) {
        if (ssa->insns[k].gpr_def == reg) {
            t->insn_type[k].is_array = 1;
            if (!t->insn_type[k].array_stride)
                t->insn_type[k].array_stride = stride;
            return;
        }
    }
}

int types_pass_array(Types *t) {
    const Ssa *ssa = t->ssa;

    for (size_t bi = 0; bi < ssa->cfg->block_count; bi++) {
        const CfgBlock *cb = &ssa->cfg->blocks[bi];
        const SsaBlockInfo *b = &ssa->blocks[bi];
        int in_loop = cb->is_loop_header || cb->loop_depth > 0;

        /* Case A — induction step: addi rY, rY, K inside a loop, and
         * rY is used as a load base somewhere in the block. */
        if (in_loop) {
            for (size_t k = 0; k < b->insn_count; k++) {
                size_t idx = b->first_insn + k;
                const SsaInsn *si = &ssa->insns[idx];
                if (si->in.op != PPC_OP_ADDI) continue;
                if (si->in.ra != si->in.rd)   continue;
                if (si->in.imm <= 0 || si->in.imm > 4096) continue;

                uint8_t reg = si->in.rd;
                uint8_t w   = 0;
                for (size_t j = 0; j < b->insn_count; j++) {
                    size_t jdx = b->first_insn + j;
                    const SsaInsn *sj = &ssa->insns[jdx];
                    if (sj->in.ra != reg) continue;
                    uint8_t eW = elem_width(sj->in.op);
                    if (eW) { w = eW; break; }
                }
                if (w == 0) continue;

                types_merge(&t->insn_type[idx],
                            (TypeInfo){0});
                t->insn_type[idx].is_array = 1;
                t->insn_type[idx].array_stride = (uint16_t)si->in.imm;
                mark_base_as_array(t, b, idx, reg,
                                   (uint16_t)si->in.imm);
            }
        }

        /* Case B — multiple loads off the same base at distinct
         * constant offsets within one block: signals struct or array. */
        uint8_t seen_off_for_reg[SSA_MAX_GPR];
        int32_t first_off[SSA_MAX_GPR];
        memset(seen_off_for_reg, 0, sizeof(seen_off_for_reg));
        for (size_t k = 0; k < b->insn_count; k++) {
            size_t idx = b->first_insn + k;
            const SsaInsn *si = &ssa->insns[idx];
            if (elem_width(si->in.op) == 0) continue;
            uint8_t ra = si->in.ra;
            if (ra >= SSA_MAX_GPR || ra == 1) continue;
            if (!seen_off_for_reg[ra]) {
                seen_off_for_reg[ra] = 1;
                first_off[ra] = si->in.imm;
            } else if (first_off[ra] != si->in.imm) {
                int32_t diff = si->in.imm - first_off[ra];
                if (diff < 0) diff = -diff;
                if (diff > 0 && diff <= 4096)
                    mark_base_as_array(t, b, idx, ra, (uint16_t)diff);
            }
        }
    }
    return 0;
}
