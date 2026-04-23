/*
 * Requirement 2.4 — build expression trees by inlining register defs
 * into their consumers.
 *
 * We walk each block forward maintaining a per-register "current
 * expression" shadow (gstr[32], fstr[32]). On every defining
 * instruction, we emit an expression string that references the
 * shadow values of its operands, then update the shadow for the
 * defined register. The result is a C-like rendering attached to
 * each SsaInsn via ssa_internf().
 *
 * We deliberately reset the shadows at block entry so phi regions
 * don't smear expressions across joins. Cross-block inlining is left
 * to a later pass that can consult phi functions.
 */
#include "ssa_internal.h"

#include <stdio.h>
#include <string.h>

static const char *gpr_name(Ssa *ssa, unsigned r) {
    return ssa_internf(ssa, "r%u", r);
}
static const char *fpr_name(Ssa *ssa, unsigned r) {
    return ssa_internf(ssa, "f%u", r);
}

static const char *render_slot_ref(Ssa *ssa, const SsaInsn *si, int is_load) {
    (void)is_load;
    const SsaStackSlot *s = &ssa->slots[si->slot_id];
    return s->name;
}

static const char *render_load(Ssa *ssa, const SsaInsn *si, const char *base) {
    if (si->slot_id != SSA_NO_VAL) return render_slot_ref(ssa, si, 1);
    const char *ty = "uint32_t";
    switch (si->in.op) {
        case PPC_OP_LBZ: ty = "uint8_t";  break;
        case PPC_OP_LHZ: ty = "uint16_t"; break;
        case PPC_OP_LFS: ty = "float";    break;
        case PPC_OP_LFD: ty = "double";   break;
        default: break;
    }
    return ssa_internf(ssa, "*(%s *)((char *)%s + %d)", ty, base, si->in.imm);
}

static const char *render_store(Ssa *ssa, const SsaInsn *si,
                                const char *base, const char *val) {
    if (si->slot_id != SSA_NO_VAL)
        return ssa_internf(ssa, "%s = %s",
                           render_slot_ref(ssa, si, 0), val);
    const char *ty = "uint32_t";
    switch (si->in.op) {
        case PPC_OP_STB: ty = "uint8_t";  break;
        case PPC_OP_STH: ty = "uint16_t"; break;
        case PPC_OP_STFS: ty = "float";   break;
        case PPC_OP_STFD: ty = "double";  break;
        default: break;
    }
    return ssa_internf(ssa, "*(%s *)((char *)%s + %d) = %s",
                       ty, base, si->in.imm, val);
}

static const char *render_call(Ssa *ssa, const SsaInsn *si,
                               const char *const *g, const char *const *f) {
    const SsaCallSite *cs = &ssa->calls[si->call_site_id];
    char buf[512];
    int n = 0;
    n += snprintf(buf + n, sizeof(buf) - n, "call_0x%08x(", cs->target);
    int first = 1;
    for (unsigned r = 3; r <= 10; r++) {
        if (!(cs->gpr_arg_mask & (1u << (r - 3)))) continue;
        n += snprintf(buf + n, sizeof(buf) - n, "%s%s",
                      first ? "" : ", ", g[r]);
        first = 0;
    }
    for (unsigned r = 1; r <= 8; r++) {
        if (!(cs->fpr_arg_mask & (1u << (r - 1)))) continue;
        n += snprintf(buf + n, sizeof(buf) - n, "%s%s",
                      first ? "" : ", ", f[r]);
        first = 0;
    }
    snprintf(buf + n, sizeof(buf) - n, ")");
    return ssa_intern(ssa, buf);
}

int ssa_build_expressions(Ssa *ssa) {
    for (size_t bi = 0; bi < ssa->cfg->block_count; bi++) {
        SsaBlockInfo *b = &ssa->blocks[bi];
        const char *g[SSA_MAX_GPR];
        const char *f[SSA_MAX_FPR];
        for (unsigned r = 0; r < SSA_MAX_GPR; r++) g[r] = gpr_name(ssa, r);
        for (unsigned r = 0; r < SSA_MAX_FPR; r++) f[r] = fpr_name(ssa, r);

        for (size_t k = 0; k < b->insn_count; k++) {
            SsaInsn *si = &ssa->insns[b->first_insn + k];
            const PpcInsn *in = &si->in;
            const char *e = NULL;

            if (si->const_valid) {
                e = ssa_internf(ssa, "0x%08x", si->const_value);
                if (si->gpr_def < SSA_MAX_GPR) g[si->gpr_def] = e;
                si->expr = e;
                continue;
            }

            switch (in->op) {
                case PPC_OP_ADDI:
                    e = ssa_internf(ssa, "(%s + %d)",
                                    in->ra ? g[in->ra] : "0", in->imm);
                    g[in->rd] = e; break;
                case PPC_OP_ADDIS:
                    e = ssa_internf(ssa, "(%s + 0x%x)",
                                    in->ra ? g[in->ra] : "0",
                                    (uint32_t)in->imm << 16);
                    g[in->rd] = e; break;
                case PPC_OP_MULLI:
                    e = ssa_internf(ssa, "(%s * %d)", g[in->ra], in->imm);
                    g[in->rd] = e; break;
                case PPC_OP_ORI:
                    e = ssa_internf(ssa, "(%s | 0x%x)",
                                    g[in->rd], (uint32_t)in->imm);
                    g[in->ra] = e; break;
                case PPC_OP_ANDI_DOT:
                    e = ssa_internf(ssa, "(%s & 0x%x)",
                                    g[in->rd], (uint32_t)in->imm);
                    g[in->ra] = e; break;
                case PPC_OP_MR:
                    e = g[in->ra];
                    g[in->rd] = e; break;
                case PPC_OP_LWZ: case PPC_OP_LBZ: case PPC_OP_LHZ:
                    e = render_load(ssa, si, in->ra ? g[in->ra] : "0");
                    g[in->rd] = e; break;
                case PPC_OP_STW: case PPC_OP_STB: case PPC_OP_STH:
                    e = render_store(ssa, si,
                                     in->ra ? g[in->ra] : "0", g[in->rd]);
                    break;
                case PPC_OP_LFS: case PPC_OP_LFD:
                    e = render_load(ssa, si, in->ra ? g[in->ra] : "0");
                    f[in->rd] = e; break;
                case PPC_OP_STFS: case PPC_OP_STFD:
                    e = render_store(ssa, si,
                                     in->ra ? g[in->ra] : "0", f[in->rd]);
                    break;
                case PPC_OP_FMR:
                    e = f[in->rb]; f[in->rd] = e; break;
                case PPC_OP_FNEG:
                    e = ssa_internf(ssa, "-(%s)", f[in->rb]);
                    f[in->rd] = e; break;
                case PPC_OP_FADD:
                    e = ssa_internf(ssa, "(%s + %s)", f[in->ra], f[in->rb]);
                    f[in->rd] = e; break;
                case PPC_OP_FSUB:
                    e = ssa_internf(ssa, "(%s - %s)", f[in->ra], f[in->rb]);
                    f[in->rd] = e; break;
                case PPC_OP_FMUL:
                    e = ssa_internf(ssa, "(%s * %s)", f[in->ra], f[in->rb]);
                    f[in->rd] = e; break;
                case PPC_OP_FDIV:
                    e = ssa_internf(ssa, "(%s / %s)", f[in->ra], f[in->rb]);
                    f[in->rd] = e; break;
                case PPC_OP_MFLR: e = "lr";  g[in->rd] = e; break;
                case PPC_OP_MFCTR: e = "ctr"; g[in->rd] = e; break;
                case PPC_OP_BL: case PPC_OP_BLA:
                    e = render_call(ssa, si, g, f);
                    if (ssa->calls[si->call_site_id].returns_gpr) g[3] = e;
                    if (ssa->calls[si->call_site_id].returns_fpr) f[1] = e;
                    break;
                default: break;
            }
            si->expr = e;
        }
    }
    return 0;
}
