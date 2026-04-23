/*
 * Requirement 3.3 — propagate parameter and return types at every
 * call site from a known-extern table.
 *
 * The extern table itself is produced by requirement 6 (RVL SDK
 * harvester). This pass is deliberately usable *without* such a
 * table: if `tab` is NULL or empty, we only refine types at calls to
 * symbols whose names we already know via the symbol table.
 */
#include "types_internal.h"

#include <string.h>

static const TypesExternSig *lookup_sig(const TypesExternTable *tab,
                                        uint32_t target, const char *name) {
    if (!tab) return NULL;
    for (size_t i = 0; i < tab->count; i++) {
        if (target && tab->sigs[i].vaddr == target) return &tab->sigs[i];
        if (name && tab->sigs[i].name &&
            strcmp(tab->sigs[i].name, name) == 0) return &tab->sigs[i];
    }
    return NULL;
}

static void apply_arg_type(Types *t, const SsaBlockInfo *b,
                           size_t call_idx,
                           uint8_t gpr_reg, uint8_t fpr_reg,
                           TypeInfo ty) {
    /* Walk backward within the block to find the last defining
     * SsaInsn for the arg register, and merge the parameter type in.
     * Arg-setup usually sits right above the call in the same block. */
    const Ssa *ssa = t->ssa;
    for (size_t k = call_idx; k-- > b->first_insn;) {
        const SsaInsn *p = &ssa->insns[k];
        if (gpr_reg < SSA_MAX_GPR && p->gpr_def == gpr_reg) {
            types_merge(&t->insn_type[k], ty);
            return;
        }
        if (fpr_reg < SSA_MAX_FPR && p->fpr_def == fpr_reg) {
            types_merge(&t->insn_type[k], ty);
            return;
        }
    }
}

int types_pass_signature(Types *t, const TypesExternTable *tab) {
    const Ssa *ssa = t->ssa;
    for (size_t bi = 0; bi < ssa->cfg->block_count; bi++) {
        const SsaBlockInfo *b = &ssa->blocks[bi];
        for (size_t k = 0; k < b->insn_count; k++) {
            size_t idx = b->first_insn + k;
            const SsaInsn *si = &ssa->insns[idx];
            if (si->call_site_id == SSA_NO_VAL) continue;
            const SsaCallSite *cs = &ssa->calls[si->call_site_id];

            const char *name = NULL;
            if (t->syms) name = sym_lookup(t->syms, cs->target);
            const TypesExternSig *sig = lookup_sig(tab, cs->target, name);
            if (!sig) continue;

            /* Return type. */
            if (cs->returns_gpr || cs->returns_fpr)
                types_merge(&t->insn_type[idx], sig->return_type);

            /* Parameters: positional. We fill GPR args first, then
             * FPR args, matching the SysV PPC convention. */
            unsigned pi = 0;
            for (unsigned r = 3; r <= 10 && pi < sig->param_count; r++) {
                if (!(cs->gpr_arg_mask & (1u << (r - 3)))) continue;
                apply_arg_type(t, b, idx, (uint8_t)r, SSA_NO_REG,
                               sig->params[pi]);
                pi++;
            }
            for (unsigned r = 1; r <= 8 && pi < sig->param_count; r++) {
                if (!(cs->fpr_arg_mask & (1u << (r - 1)))) continue;
                apply_arg_type(t, b, idx, SSA_NO_REG, (uint8_t)r,
                               sig->params[pi]);
                pi++;
            }
        }
    }
    return 0;
}
