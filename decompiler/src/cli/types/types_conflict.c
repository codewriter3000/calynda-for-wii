/*
 * Requirement 3.6 — type-conflict resolution.
 *
 * Each preceding pass calls types_merge, which already widens on
 * disagreement and sets the per-entry conflict flag. This final pass
 * walks the three tables, counts entries with conflict == 1 for
 * reporting, and applies a couple of cleanup rules:
 *
 *   - An int type with width 0 defaults to width 4 (natural GPR).
 *   - A pointer with no width becomes width 4.
 *   - Merge slot type back into every load/store instruction that
 *     touches the slot, so a disagreement between two producers is
 *     reflected everywhere and shows up in later C emission as a cast.
 */
#include "types_internal.h"

int types_pass_conflict(Types *t) {
    const Ssa *ssa = t->ssa;

    /* Propagate slot types to all touching instructions. */
    for (size_t i = 0; i < ssa->insn_count; i++) {
        const SsaInsn *si = &ssa->insns[i];
        if (si->slot_id == SSA_NO_VAL) continue;
        types_merge(&t->insn_type[i], t->slot_type[si->slot_id]);
        /* Slot picks up the combined view too. */
        types_merge(&t->slot_type[si->slot_id], t->insn_type[i]);
    }

    /* Normalize widths and count conflicts. */
    unsigned n = 0;
    for (size_t i = 0; i < ssa->insn_count; i++) {
        TypeInfo *ti = &t->insn_type[i];
        if (ti->kind == TYPE_INT && ti->width == 0) ti->width = 4;
        if (ti->kind == TYPE_PTR && ti->width == 0) ti->width = 4;
        if (ti->conflict) n++;
    }
    for (size_t i = 0; i < ssa->slot_count; i++) {
        TypeInfo *ti = &t->slot_type[i];
        if (ti->kind == TYPE_INT && ti->width == 0) ti->width = 4;
        if (ti->kind == TYPE_PTR && ti->width == 0) ti->width = 4;
        if (ti->conflict) n++;
    }
    for (size_t i = 0; i < ssa->phi_count; i++) {
        TypeInfo *ti = &t->phi_type[i];
        if (ti->kind == TYPE_INT && ti->width == 0) ti->width = 4;
        if (ti->conflict) n++;
    }
    t->conflicts = n;
    return 0;
}
