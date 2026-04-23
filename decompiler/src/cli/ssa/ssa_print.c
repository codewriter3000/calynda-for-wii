/*
 * Pretty-printer for the SSA side tables.
 */
#include "ssa_internal.h"

#include <inttypes.h>
#include <stdio.h>

static void print_bits_gpr(FILE *out, const char *label, uint64_t m) {
    if (m == 0) return;
    fprintf(out, "    %s:", label);
    for (unsigned r = 0; r < SSA_MAX_GPR; r++)
        if (m & (1ULL << r)) fprintf(out, " r%u", r);
    fputc('\n', out);
}

static void print_bits_fpr(FILE *out, const char *label, uint64_t m) {
    if (m == 0) return;
    fprintf(out, "    %s:", label);
    for (unsigned r = 0; r < SSA_MAX_FPR; r++)
        if (m & (1ULL << r)) fprintf(out, " f%u", r);
    fputc('\n', out);
}

void ssa_print(const Ssa *ssa, FILE *out) {
    fprintf(out, "# SSA: %zu insns, %zu blocks, %zu phis, "
                 "%zu slots, %zu calls\n",
            ssa->insn_count, ssa->cfg->block_count,
            ssa->phi_count, ssa->slot_count, ssa->call_count);

    for (size_t bi = 0; bi < ssa->cfg->block_count; bi++) {
        const CfgBlock *cb = &ssa->cfg->blocks[bi];
        const SsaBlockInfo *b = &ssa->blocks[bi];
        fprintf(out, "\nB%zu 0x%08x..0x%08x  (%u insns)\n",
                bi, cb->start_vaddr, cb->end_vaddr, cb->insn_count);
        print_bits_gpr(out, "gpr_defs      ", b->gpr_defs);
        print_bits_gpr(out, "gpr_use-bd    ", b->gpr_uses_before_def);
        print_bits_gpr(out, "gpr_live_in   ", b->gpr_live_in);
        print_bits_gpr(out, "gpr_phi       ", b->gpr_phi);
        print_bits_fpr(out, "fpr_defs      ", b->fpr_defs);
        print_bits_fpr(out, "fpr_use-bd    ", b->fpr_uses_before_def);
        print_bits_fpr(out, "fpr_live_in   ", b->fpr_live_in);
        print_bits_fpr(out, "fpr_phi       ", b->fpr_phi);

        for (size_t k = 0; k < b->insn_count; k++) {
            const SsaInsn *si = &ssa->insns[b->first_insn + k];
            fprintf(out, "    0x%08x  %-48s",
                    si->pc, si->expr ? si->expr : "");
            if (si->gpr_def < SSA_MAX_GPR) fprintf(out, " -> r%u", si->gpr_def);
            if (si->fpr_def < SSA_MAX_FPR) fprintf(out, " -> f%u", si->fpr_def);
            if (si->const_valid)
                fprintf(out, "  [const=0x%08x]", si->const_value);
            if (si->slot_id != SSA_NO_VAL)
                fprintf(out, "  [%s]", ssa->slots[si->slot_id].name);
            if (si->call_site_id != SSA_NO_VAL) {
                const SsaCallSite *cs = &ssa->calls[si->call_site_id];
                fprintf(out, "  [call gpr=0x%02x fpr=0x%02x rv_g=%u rv_f=%u]",
                        cs->gpr_arg_mask, cs->fpr_arg_mask,
                        cs->returns_gpr, cs->returns_fpr);
            }
            fputc('\n', out);
        }
    }

    if (ssa->phi_count) {
        fprintf(out, "\nphis:\n");
        for (size_t i = 0; i < ssa->phi_count; i++) {
            const SsaPhi *p = &ssa->phis[i];
            fprintf(out, "  B%u: %s%u = phi(...)\n",
                    p->block, p->reg_kind ? "f" : "r", p->reg);
        }
    }

    if (ssa->slot_count) {
        fprintf(out, "\nstack slots:\n");
        for (size_t i = 0; i < ssa->slot_count; i++) {
            const SsaStackSlot *s = &ssa->slots[i];
            fprintf(out, "  %-12s off=%+d w=%u %s  loads=%u stores=%u\n",
                    s->name, s->offset, s->width,
                    s->is_float ? "f" : "i",
                    s->load_count, s->store_count);
        }
    }

    if (ssa->call_count) {
        fprintf(out, "\ncall sites:\n");
        for (size_t i = 0; i < ssa->call_count; i++) {
            const SsaCallSite *cs = &ssa->calls[i];
            fprintf(out, "  0x%08x -> 0x%08x  gprs=0x%02x fprs=0x%02x"
                         "  rv_g=%u rv_f=%u\n",
                    ssa->insns[cs->insn_index].pc, cs->target,
                    cs->gpr_arg_mask, cs->fpr_arg_mask,
                    cs->returns_gpr, cs->returns_fpr);
        }
    }
}
