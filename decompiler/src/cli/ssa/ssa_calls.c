/*
 * Requirement 2.6 — for every call instruction, reconstruct the set
 * of positional arguments and whether the return value is consumed.
 *
 * Convention (PPC SysV / EABI):
 *   - GPR args: r3 .. r10 (up to 8).
 *   - FPR args: f1 .. f8  (up to 8).
 *   - GPR return: r3.
 *   - FPR return: f1.
 *
 * Heuristic: a register is considered an argument if some
 * instruction preceding the call within the same block defined it,
 * OR if it is live-in to the block containing the call. That
 * reaches both local setup (most common) and args established in a
 * dominating predecessor.
 *
 * The return value is considered consumed if r3 / f1 is used before
 * the next write within the same block, or if it is live-out from
 * the block.
 */
#include "ssa_internal.h"

#include <stdlib.h>
#include <string.h>

#define BIT(i) (1ULL << (unsigned)(i))

static uint64_t live_out_gpr(const Ssa *ssa, uint32_t bi) {
    uint64_t live = 0;
    for (size_t e = 0; e < ssa->cfg->edge_count; e++) {
        const CfgEdge *ed = &ssa->cfg->edges[e];
        if (ed->from != bi || ed->to == CFG_INVALID) continue;
        live |= ssa->blocks[ed->to].gpr_live_in;
    }
    return live;
}

static uint64_t live_out_fpr(const Ssa *ssa, uint32_t bi) {
    uint64_t live = 0;
    for (size_t e = 0; e < ssa->cfg->edge_count; e++) {
        const CfgEdge *ed = &ssa->cfg->edges[e];
        if (ed->from != bi || ed->to == CFG_INVALID) continue;
        live |= ssa->blocks[ed->to].fpr_live_in;
    }
    return live;
}

static int register_site(Ssa *ssa, uint32_t insn_idx,
                         uint8_t garg, uint8_t farg,
                         uint8_t rv_g, uint8_t rv_f) {
    size_t n = ssa->call_count + 1;
    SsaCallSite *np = (SsaCallSite *)realloc(ssa->calls,
                                             n * sizeof(SsaCallSite));
    if (!np) return -1;
    ssa->calls = np;
    SsaCallSite *cs = &ssa->calls[ssa->call_count];
    cs->insn_index   = insn_idx;
    cs->target       = ssa->insns[insn_idx].in.target;
    cs->gpr_arg_mask = garg;
    cs->fpr_arg_mask = farg;
    cs->returns_gpr  = rv_g;
    cs->returns_fpr  = rv_f;
    ssa->insns[insn_idx].call_site_id = (uint32_t)ssa->call_count;
    ssa->call_count++;
    return 0;
}

int ssa_reconstruct_calls(Ssa *ssa) {
    for (size_t bi = 0; bi < ssa->cfg->block_count; bi++) {
        SsaBlockInfo *b = &ssa->blocks[bi];
        uint64_t gdefs = 0, fdefs = 0;

        for (size_t k = 0; k < b->insn_count; k++) {
            size_t idx = b->first_insn + k;
            SsaInsn *si = &ssa->insns[idx];

            if (si->in.op == PPC_OP_BL || si->in.op == PPC_OP_BLA) {
                /* Arg set: preceding block defs union live-in. */
                uint64_t g_avail = gdefs | b->gpr_live_in;
                uint64_t f_avail = fdefs | b->fpr_live_in;
                uint8_t garg = 0;
                for (unsigned r = 3; r <= 10; r++)
                    if (g_avail & BIT(r)) garg |= (uint8_t)(1u << (r - 3));
                uint8_t farg = 0;
                for (unsigned r = 1; r <= 8; r++)
                    if (f_avail & BIT(r)) farg |= (uint8_t)(1u << (r - 1));

                /* Return value: is r3 / f1 used or live-out after this? */
                uint64_t g_redef = 0, f_redef = 0;
                int g_used_after = 0, f_used_after = 0;
                for (size_t j = k + 1; j < b->insn_count; j++) {
                    SsaInsn *nx = &ssa->insns[b->first_insn + j];
                    if (nx->gpr_uses & BIT(3) & ~g_redef) g_used_after = 1;
                    if (nx->fpr_uses & BIT(1) & ~f_redef) f_used_after = 1;
                    if (nx->gpr_def < SSA_MAX_GPR)
                        g_redef |= BIT(nx->gpr_def);
                    if (nx->fpr_def < SSA_MAX_FPR)
                        f_redef |= BIT(nx->fpr_def);
                }
                uint64_t lo_g = live_out_gpr(ssa, (uint32_t)bi);
                uint64_t lo_f = live_out_fpr(ssa, (uint32_t)bi);
                uint8_t rv_g = (uint8_t)(g_used_after ||
                    ((lo_g & BIT(3)) && !(g_redef & BIT(3))));
                uint8_t rv_f = (uint8_t)(f_used_after ||
                    ((lo_f & BIT(1)) && !(f_redef & BIT(1))));

                if (register_site(ssa, (uint32_t)idx,
                                  garg, farg, rv_g, rv_f) != 0) return -1;
            }

            if (si->gpr_def < SSA_MAX_GPR) gdefs |= BIT(si->gpr_def);
            if (si->fpr_def < SSA_MAX_FPR) fdefs |= BIT(si->fpr_def);
        }
    }
    return 0;
}
