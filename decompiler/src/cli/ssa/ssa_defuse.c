/*
 * Requirement 2.1 — compute per-instruction register effects, then
 * fold them into per-block def / use-before-def / live-in summaries.
 */
#include "ssa_internal.h"

#include <stdlib.h>
#include <string.h>

#define BIT(i) (1ULL << (unsigned)(i))

static void set_gpr_use(SsaInsn *si, unsigned r) {
    if (r < SSA_MAX_GPR) si->gpr_uses |= BIT(r);
}
static void set_fpr_use(SsaInsn *si, unsigned r) {
    if (r < SSA_MAX_FPR) si->fpr_uses |= BIT(r);
}

/* Derive gpr_def, gpr_uses, fpr_def, fpr_uses from a decoded insn.
 * Call-site and NOP-ish ops are handled conservatively here; the
 * calls pass refines SysV argument/return effects later. */
static void compute_insn_effects(SsaInsn *si) {
    const PpcInsn *in = &si->in;
    switch (in->op) {
        /* Arithmetic immediate: def rd, use ra (ra==0 is literal 0). */
        case PPC_OP_ADDI:
        case PPC_OP_ADDIS:
        case PPC_OP_MULLI:
            si->gpr_def = in->rd;
            if (in->ra != 0) set_gpr_use(si, in->ra);
            break;

        case PPC_OP_CMPWI:
            set_gpr_use(si, in->ra);
            break;

        /* ori rA, rS, imm  (decoder: rd=rS, ra=rA). */
        case PPC_OP_ORI:
        case PPC_OP_ANDI_DOT:
            si->gpr_def = in->ra;
            set_gpr_use(si, in->rd);
            break;

        /* mr rA, rS  (decoder: rd=rA, ra=rS). */
        case PPC_OP_MR:
            si->gpr_def = in->rd;
            set_gpr_use(si, in->ra);
            break;

        /* GPR loads. */
        case PPC_OP_LWZ: case PPC_OP_LBZ: case PPC_OP_LHZ:
        case PPC_OP_LMW:
            si->gpr_def = in->rd;
            if (in->ra != 0) set_gpr_use(si, in->ra);
            break;

        /* GPR stores. */
        case PPC_OP_STW: case PPC_OP_STB: case PPC_OP_STH:
        case PPC_OP_STMW:
            set_gpr_use(si, in->rd);
            if (in->ra != 0) set_gpr_use(si, in->ra);
            break;

        /* Float loads (GPR base, FPR dest). */
        case PPC_OP_LFS: case PPC_OP_LFD:
            si->fpr_def = in->rd;
            if (in->ra != 0) set_gpr_use(si, in->ra);
            break;

        /* Float stores. */
        case PPC_OP_STFS: case PPC_OP_STFD:
            set_fpr_use(si, in->rd);
            if (in->ra != 0) set_gpr_use(si, in->ra);
            break;

        case PPC_OP_FMR:
        case PPC_OP_FNEG:
            si->fpr_def = in->rd;
            set_fpr_use(si, in->rb);
            break;

        case PPC_OP_FADD:
        case PPC_OP_FSUB:
        case PPC_OP_FDIV:
        case PPC_OP_FMUL:
            si->fpr_def = in->rd;
            set_fpr_use(si, in->ra);
            set_fpr_use(si, in->rb);
            break;

        case PPC_OP_MFLR:
        case PPC_OP_MFCTR:
            si->gpr_def = in->rd;
            break;

        case PPC_OP_MTLR:
        case PPC_OP_MTCTR:
            set_gpr_use(si, in->rd);
            break;

        /* Calls define the SysV return registers conservatively;
         * ssa_reconstruct_calls narrows the arg mask later. */
        case PPC_OP_BL:
        case PPC_OP_BLA:
            si->gpr_def = 3;
            break;

        default:
            break;
    }
}

int ssa_compute_defuse(Ssa *ssa) {
    /* Per-instruction effects. */
    for (size_t i = 0; i < ssa->insn_count; i++)
        compute_insn_effects(&ssa->insns[i]);

    /* Per-block summaries + last-def tracking. */
    for (size_t bi = 0; bi < ssa->cfg->block_count; bi++) {
        SsaBlockInfo *b = &ssa->blocks[bi];
        uint64_t defs = 0, uses_bd = 0, fdefs = 0, fuses_bd = 0;
        for (size_t k = 0; k < b->insn_count; k++) {
            SsaInsn *si = &ssa->insns[b->first_insn + k];
            uint64_t g_new = si->gpr_uses & ~defs;
            uint64_t f_new = si->fpr_uses & ~fdefs;
            uses_bd  |= g_new;
            fuses_bd |= f_new;
            if (si->gpr_def < SSA_MAX_GPR) {
                defs |= BIT(si->gpr_def);
                b->gpr_last_def[si->gpr_def] = (uint32_t)(b->first_insn + k);
            }
            if (si->fpr_def < SSA_MAX_FPR) {
                fdefs |= BIT(si->fpr_def);
                b->fpr_last_def[si->fpr_def] = (uint32_t)(b->first_insn + k);
            }
        }
        b->gpr_defs = defs;
        b->fpr_defs = fdefs;
        b->gpr_uses_before_def = uses_bd;
        b->fpr_uses_before_def = fuses_bd;
        b->gpr_live_in = uses_bd;
        b->fpr_live_in = fuses_bd;
    }

    /* Iterative live-in: live_in[b] = use_bd[b] | (union over succs
     * of live_in[s]) & ~def[b]. Reverse-iterate for faster fixpoint. */
    int changed = 1;
    while (changed) {
        changed = 0;
        for (size_t bi = ssa->cfg->block_count; bi-- > 0;) {
            SsaBlockInfo *b = &ssa->blocks[bi];
            uint64_t g = 0, f = 0;
            for (size_t e = 0; e < ssa->cfg->edge_count; e++) {
                const CfgEdge *ed = &ssa->cfg->edges[e];
                if (ed->from != bi || ed->to == CFG_INVALID) continue;
                g |= ssa->blocks[ed->to].gpr_live_in;
                f |= ssa->blocks[ed->to].fpr_live_in;
            }
            uint64_t new_g = b->gpr_uses_before_def | (g & ~b->gpr_defs);
            uint64_t new_f = b->fpr_uses_before_def | (f & ~b->fpr_defs);
            if (new_g != b->gpr_live_in || new_f != b->fpr_live_in) {
                b->gpr_live_in = new_g;
                b->fpr_live_in = new_f;
                changed = 1;
            }
        }
    }
    return 0;
}
