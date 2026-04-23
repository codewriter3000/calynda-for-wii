/*
 * Requirement 3.2 — pointer arithmetic detection.
 *
 * Whenever a register is loaded with a 32-bit constant that matches
 * a known global symbol, tag the defining instruction's type as
 * `ptr_to_sym`. When the constant is *close to* a symbol (within its
 * size range), tag as a pointer into that symbol.
 *
 * We also propagate: if an `addi rD, rA, imm` consumes a pointer-typed
 * source, the result is a pointer-typed offset into that same symbol.
 */
#include "types_internal.h"

#include <string.h>

/* Find a symbol whose [vaddr, vaddr+size) contains `addr`. */
static const SymEntry *find_containing_sym(const SymTable *syms,
                                           uint32_t addr) {
    if (!syms) return NULL;
    return sym_covering(syms, addr);
}

int types_pass_ptr(Types *t) {
    const Ssa *ssa = t->ssa;
    for (size_t i = 0; i < ssa->insn_count; i++) {
        const SsaInsn *si = &ssa->insns[i];
        if (!si->const_valid) continue;
        /* Heuristic: values in the mainram / IPL ranges that match a
         * symbol are very likely pointers; non-matching constants are
         * left as ints for pass_sign to classify. */
        uint32_t v = si->const_value;
        if (v < 0x80000000u || v >= 0x81800000u) continue;
        const SymEntry *e = find_containing_sym(t->syms, v);
        if (!e) continue;
        TypeInfo p = {0};
        p.kind = TYPE_PTR;
        p.width = 4;
        p.ptr_to_sym = e->name;
        types_merge(&t->insn_type[i], p);
    }

    /* Forward-propagate pointer-ness through addi within each block.
     * We keep a per-GPR shadow of the current pointer type and clear
     * on any non-pointer redefining instruction. */
    for (size_t bi = 0; bi < ssa->cfg->block_count; bi++) {
        const SsaBlockInfo *b = &ssa->blocks[bi];
        TypeInfo gpr[SSA_MAX_GPR] = {0};

        for (size_t k = 0; k < b->insn_count; k++) {
            size_t idx = b->first_insn + k;
            const SsaInsn *si = &ssa->insns[idx];
            const PpcInsn *in = &si->in;

            /* Seed from the pointer types this pass already inferred. */
            if (t->insn_type[idx].kind == TYPE_PTR &&
                si->gpr_def < SSA_MAX_GPR) {
                gpr[si->gpr_def] = t->insn_type[idx];
            }

            switch (in->op) {
                case PPC_OP_ADDI:
                case PPC_OP_ORI:
                    if (in->ra < SSA_MAX_GPR &&
                        gpr[in->ra].kind == TYPE_PTR) {
                        /* Propagate to the destination register. */
                        uint8_t dst = (in->op == PPC_OP_ORI) ? in->ra : in->rd;
                        (void)dst;
                        types_merge(&t->insn_type[idx], gpr[in->ra]);
                        if (si->gpr_def < SSA_MAX_GPR)
                            gpr[si->gpr_def] = t->insn_type[idx];
                    }
                    break;

                case PPC_OP_MR:
                    if (in->ra < SSA_MAX_GPR &&
                        gpr[in->ra].kind == TYPE_PTR) {
                        types_merge(&t->insn_type[idx], gpr[in->ra]);
                        if (si->gpr_def < SSA_MAX_GPR)
                            gpr[si->gpr_def] = t->insn_type[idx];
                    }
                    break;

                default:
                    /* Any other defining op clears the shadow. */
                    if (si->gpr_def < SSA_MAX_GPR &&
                        t->insn_type[idx].kind != TYPE_PTR) {
                        memset(&gpr[si->gpr_def], 0, sizeof(TypeInfo));
                    }
                    break;
            }
        }
    }
    return 0;
}
