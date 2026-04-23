/*
 * Requirement 3.4 — signedness inference.
 *
 * PowerPC cues:
 *   lbz / lhz / lwz  -> zero-extended loads (unsigned).
 *   lha / lhau       -> sign-extended (signed). (Not in the decoder
 *                       yet; we will opportunistically flag them if
 *                       they appear in the raw opcode bits.)
 *   cmpwi            -> signed comparison. Any register fed into
 *                       cmpwi takes on signed.
 *   cmplwi (opc 11, L=0, u=1) -> unsigned compare; decoder currently
 *                       exposes only cmpwi.
 *   mulli            -> signed immediate multiply.
 *
 * Propagation is forward through arithmetic with a single source, and
 * backward through cmpwi uses within the same block. Conflicts are
 * recorded via types_merge.
 */
#include "types_internal.h"

#include <string.h>

static TypeInfo signed_int(uint8_t w) {
    TypeInfo t = {0};
    t.kind = TYPE_INT;
    t.width = w;
    t.sign = SIGN_SIGNED;
    return t;
}
static TypeInfo unsigned_int(uint8_t w) {
    TypeInfo t = {0};
    t.kind = TYPE_INT;
    t.width = w;
    t.sign = SIGN_UNSIGNED;
    return t;
}

/* Walk back in the block to find the last def for register `r` and
 * merge `ty` into its insn_type. Stops at the block boundary. */
static void back_prop(Types *t, const SsaBlockInfo *b,
                      size_t from, uint8_t reg, TypeInfo ty) {
    const Ssa *ssa = t->ssa;
    for (size_t k = from; k-- > b->first_insn;) {
        if (ssa->insns[k].gpr_def == reg) {
            types_merge(&t->insn_type[k], ty);
            return;
        }
    }
}

int types_pass_sign(Types *t) {
    const Ssa *ssa = t->ssa;

    /* Forward: loads are unsigned; mulli is signed. */
    for (size_t i = 0; i < ssa->insn_count; i++) {
        const SsaInsn *si = &ssa->insns[i];
        switch (si->in.op) {
            case PPC_OP_LBZ:  types_merge(&t->insn_type[i], unsigned_int(1)); break;
            case PPC_OP_LHZ:  types_merge(&t->insn_type[i], unsigned_int(2)); break;
            case PPC_OP_LWZ:  types_merge(&t->insn_type[i], unsigned_int(4)); break;
            case PPC_OP_MULLI: types_merge(&t->insn_type[i], signed_int(4)); break;
            /* addi with a negative immediate suggests signed arithmetic. */
            case PPC_OP_ADDI:
                if (si->in.imm < 0)
                    types_merge(&t->insn_type[i], signed_int(4));
                break;
            default: break;
        }
    }

    /* Backward: cmpwi forces its ra operand to signed. */
    for (size_t bi = 0; bi < ssa->cfg->block_count; bi++) {
        const SsaBlockInfo *b = &ssa->blocks[bi];
        for (size_t k = 0; k < b->insn_count; k++) {
            size_t idx = b->first_insn + k;
            const SsaInsn *si = &ssa->insns[idx];
            if (si->in.op == PPC_OP_CMPWI) {
                back_prop(t, b, idx, si->in.ra, signed_int(4));
            }
        }
    }

    /* Forward: mr / ori single-source arithmetic inherits sign. */
    for (size_t bi = 0; bi < ssa->cfg->block_count; bi++) {
        const SsaBlockInfo *b = &ssa->blocks[bi];
        Signedness sh[SSA_MAX_GPR];
        memset(sh, 0, sizeof(sh));
        for (size_t k = 0; k < b->insn_count; k++) {
            size_t idx = b->first_insn + k;
            const SsaInsn *si = &ssa->insns[idx];
            if (t->insn_type[idx].sign != SIGN_UNKNOWN &&
                si->gpr_def < SSA_MAX_GPR) {
                sh[si->gpr_def] = t->insn_type[idx].sign;
            }
            if ((si->in.op == PPC_OP_MR || si->in.op == PPC_OP_ORI) &&
                si->in.ra < SSA_MAX_GPR && sh[si->in.ra] != SIGN_UNKNOWN) {
                TypeInfo p = {0};
                p.kind = TYPE_INT;
                p.width = 4;
                p.sign = sh[si->in.ra];
                types_merge(&t->insn_type[idx], p);
                if (si->gpr_def < SSA_MAX_GPR) sh[si->gpr_def] = p.sign;
            }
        }
    }

    return 0;
}
