/*
 * Types module orchestrator, merge helper, pretty-printer, and
 * format helper.
 */
#include "types_internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int types_merge(TypeInfo *dst, const TypeInfo src) {
    TypeInfo before = *dst;

    /* Kind merge. */
    if (dst->kind == TYPE_UNKNOWN)         dst->kind = src.kind;
    else if (src.kind == TYPE_UNKNOWN)     {}
    else if (dst->kind != src.kind) {
        /* int vs ptr at 4 bytes -> prefer ptr. Anything else is a
         * conflict and we widen to int. */
        if ((dst->kind == TYPE_INT && src.kind == TYPE_PTR) ||
            (dst->kind == TYPE_PTR && src.kind == TYPE_INT)) {
            dst->kind = TYPE_PTR;
            if (!dst->ptr_to_sym) dst->ptr_to_sym = src.ptr_to_sym;
        } else {
            dst->conflict = 1;
            dst->kind = TYPE_INT;  /* neutral fallback                */
        }
    }

    /* Width: max of the two when both known; otherwise adopt known. */
    if (src.width && dst->width && src.width != dst->width) {
        dst->width = src.width > dst->width ? src.width : dst->width;
    } else if (src.width && !dst->width) {
        dst->width = src.width;
    }

    /* Sign: disagreements widen to unsigned and record a conflict. */
    if (dst->sign == SIGN_UNKNOWN)       dst->sign = src.sign;
    else if (src.sign == SIGN_UNKNOWN)   {}
    else if (dst->sign != src.sign) {
        dst->conflict = 1;
        dst->sign = SIGN_UNSIGNED;
    }

    if (src.ptr_to_sym && !dst->ptr_to_sym) dst->ptr_to_sym = src.ptr_to_sym;
    if (src.is_array && !dst->is_array) {
        dst->is_array = 1;
        dst->array_stride = src.array_stride;
    }
    dst->conflict |= src.conflict;

    return memcmp(&before, dst, sizeof(*dst)) != 0;
}

const char *types_format(const TypeInfo *ti, char *buf, size_t cap) {
    const char *k = "?";
    switch (ti->kind) {
        case TYPE_INT:   k = ti->sign == SIGN_SIGNED ? "int" : "uint"; break;
        case TYPE_FLOAT: k = "f";                                      break;
        case TYPE_PTR:   k = "ptr";                                    break;
        case TYPE_VOID:  k = "void";                                   break;
        default:                                                       break;
    }
    if (ti->kind == TYPE_PTR) {
        const char *sym = ti->ptr_to_sym ? ti->ptr_to_sym : "void";
        snprintf(buf, cap, "%s%s*%s", sym, ti->is_array ? "[]" : "",
                 ti->conflict ? " !!" : "");
    } else if (ti->kind == TYPE_INT) {
        snprintf(buf, cap, "%s%u%s%s", k, ti->width ? ti->width * 8u : 0u,
                 ti->is_array ? "[]" : "",
                 ti->conflict ? " !!" : "");
    } else if (ti->kind == TYPE_FLOAT) {
        snprintf(buf, cap, "%s%u%s", k, ti->width ? ti->width * 8u : 0u,
                 ti->conflict ? " !!" : "");
    } else {
        snprintf(buf, cap, "%s%s", k, ti->conflict ? " !!" : "");
    }
    return buf;
}

static int alloc_tables(Types *t) {
    size_t n = t->ssa->insn_count;
    t->insn_type = (TypeInfo *)calloc(n ? n : 1, sizeof(TypeInfo));
    t->phi_type  = (TypeInfo *)calloc(t->ssa->phi_count ? t->ssa->phi_count : 1,
                                      sizeof(TypeInfo));
    t->slot_type = (TypeInfo *)calloc(t->ssa->slot_count ? t->ssa->slot_count : 1,
                                      sizeof(TypeInfo));
    if (!t->insn_type || !t->phi_type || !t->slot_type) return -1;
    return 0;
}

int types_build(const Ssa *ssa, const SymTable *syms,
                const TypesExternTable *externs, Types *out) {
    if (!ssa || !out) return -1;
    memset(out, 0, sizeof(*out));
    out->ssa  = ssa;
    out->syms = syms;
    if (alloc_tables(out) != 0)            { types_free(out); return -1; }
    if (types_pass_width(out)        != 0) { types_free(out); return -1; }
    if (types_pass_ptr(out)          != 0) { types_free(out); return -1; }
    if (types_pass_signature(out, externs) != 0) {
        types_free(out); return -1;
    }
    if (types_pass_sign(out)         != 0) { types_free(out); return -1; }
    if (types_pass_array(out)        != 0) { types_free(out); return -1; }
    if (types_pass_conflict(out)     != 0) { types_free(out); return -1; }
    return 0;
}

void types_free(Types *t) {
    if (!t) return;
    free(t->insn_type);
    free(t->phi_type);
    free(t->slot_type);
    memset(t, 0, sizeof(*t));
}

void types_print(const Types *t, FILE *out) {
    fprintf(out, "# Types: %zu insn slots, %zu phis, %zu stack slots, "
                 "%u conflicts\n",
            t->ssa->insn_count, t->ssa->phi_count, t->ssa->slot_count,
            t->conflicts);

    char buf[64];
    for (size_t i = 0; i < t->ssa->insn_count; i++) {
        const SsaInsn *si = &t->ssa->insns[i];
        if (si->gpr_def >= SSA_MAX_GPR && si->fpr_def >= SSA_MAX_FPR &&
            t->insn_type[i].kind == TYPE_UNKNOWN) continue;
        fprintf(out, "  0x%08x %-20s ", si->pc,
                types_format(&t->insn_type[i], buf, sizeof(buf)));
        if (si->gpr_def < SSA_MAX_GPR) fprintf(out, "r%u", si->gpr_def);
        else if (si->fpr_def < SSA_MAX_FPR) fprintf(out, "f%u", si->fpr_def);
        fputc('\n', out);
    }

    if (t->ssa->slot_count) {
        fprintf(out, "\nstack slot types:\n");
        for (size_t i = 0; i < t->ssa->slot_count; i++) {
            fprintf(out, "  %-12s %s\n",
                    t->ssa->slots[i].name,
                    types_format(&t->slot_type[i], buf, sizeof(buf)));
        }
    }
}
