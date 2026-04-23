/*
 * Extern inventory builder (requirement 6.1 / 6.3 / 6.4 / 6.6).
 */
#include "cli/externs.h"
#include "cli/demangle.h"
#include "ppc.h"
#include "dol.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void sanitize(char *s) {
    for (; *s; s++)
        if (!(isalnum((unsigned char)*s) || *s == '_')) *s = '_';
}

static void build_calynda_name(const CwDemangled *d,
                               const char *raw,
                               const char *module,
                               char *out, size_t n) {
    if (d->ok) {
        /* Use the demangler's method name + its qualifier minus
         * leading module component. */
        char cal[256];
        cw_to_calynda_binding(d, cal, sizeof cal);
        /* Strip leading "module." so the extern lives inside the
         * module's .cal file without namespace duplication. */
        size_t ml = strlen(module);
        if (ml && strncmp(cal, module, ml) == 0 && cal[ml] == '.')
            snprintf(out, n, "%s", cal + ml + 1);
        else
            snprintf(out, n, "%s", cal);
    } else {
        snprintf(out, n, "%s", raw);
    }
    sanitize(out);
}

static int split_args(const char *args_paren, char parts[16][128]) {
    /* args_paren like "(int, const char*, ...)" — strip the parens. */
    const char *s = args_paren;
    if (*s == '(') s++;
    size_t len = strlen(s);
    char buf[384];
    if (len >= sizeof buf) len = sizeof buf - 1;
    memcpy(buf, s, len); buf[len] = 0;
    if (len && buf[len - 1] == ')') buf[len - 1] = 0;

    int count = 0;
    int depth = 0;
    const char *seg = buf;
    for (const char *p = buf; count < 16; p++) {
        if (*p == '(' || *p == '<') depth++;
        else if (*p == ')' || *p == '>') depth--;
        if ((*p == ',' && depth == 0) || *p == 0) {
            size_t l = (size_t)(p - seg);
            if (l >= 128) l = 127;
            memcpy(parts[count], seg, l); parts[count][l] = 0;
            /* trim */
            char *t = parts[count];
            while (*t == ' ') memmove(t, t + 1, strlen(t));
            size_t tl = strlen(t);
            while (tl && t[tl - 1] == ' ') t[--tl] = 0;
            if (strcmp(parts[count], "void") == 0) parts[count][0] = 0;
            if (parts[count][0]) count++;
            if (!*p) break;
            seg = p + 1;
        }
    }
    return count;
}

static void build_signature(const CwDemangled *d, const char *raw,
                            char *out, size_t n, int *is_var) {
    *is_var = 0;
    if (!d->ok) {
        /* C function: no type info, emit `fn(...) -> ()` placeholder. */
        snprintf(out, n, "fn(...args) -> ()");
        *is_var = 1;
        (void)raw;
        return;
    }
    char parts[16][128];
    int count = split_args(d->args, parts);
    char buf[512] = "";
    strncat(buf, "fn(", sizeof buf - 1);
    int first = 1;
    for (int i = 0; i < count; i++) {
        if (strcmp(parts[i], "...") == 0) {
            if (!first) strncat(buf, ", ", sizeof buf - strlen(buf) - 1);
            strncat(buf, "...args", sizeof buf - strlen(buf) - 1);
            *is_var = 1;
            continue;
        }
        char cal[128];
        externs_cpp_type_to_calynda(parts[i], cal, sizeof cal);
        if (!first) strncat(buf, ", ", sizeof buf - strlen(buf) - 1);
        char label[16]; snprintf(label, sizeof label, "a%d: ", i);
        strncat(buf, label, sizeof buf - strlen(buf) - 1);
        strncat(buf, cal, sizeof buf - strlen(buf) - 1);
        first = 0;
    }
    strncat(buf, ") -> ()", sizeof buf - strlen(buf) - 1);
    snprintf(out, n, "%s", buf);
}

static int in_text(const DolImage *img, uint32_t vaddr) {
    const DolSection *s = dol_section_for_vaddr(img, vaddr);
    return s && s->kind == DOL_SECTION_TEXT;
}

static void push(Externs *ex, ExternSymbol v) {
    if (ex->count == ex->cap) {
        ex->cap = ex->cap ? ex->cap * 2 : 64;
        ex->entries = realloc(ex->entries, ex->cap * sizeof(ExternSymbol));
    }
    ex->entries[ex->count++] = v;
}

static int already_have(const Externs *ex, uint32_t v) {
    for (size_t i = 0; i < ex->count; i++)
        if (ex->entries[i].vaddr == v) return 1;
    return 0;
}

int externs_build(const DolImage *img, const SymTable *syms, Externs *out) {
    memset(out, 0, sizeof *out);
    if (!syms) return -1;

    /* 6.1a — every symbol whose name matches an SDK prefix is a
     *        candidate extern. */
    for (size_t i = 0; i < syms->count; i++) {
        const SymEntry *e = &syms->entries[i];
        CwDemangled d;
        cw_demangle(e->name, &d);
        /* Classify by demangled qualifier first (so `adjust__Q23EGG7...`
         * lands in `egg`), falling back to the raw name. */
        const char *mod = d.ok && d.qualifier[0]
                        ? externs_classify(d.qualifier)
                        : externs_classify(e->name);
        if (!*mod) continue;
        ExternSymbol s = { 0 };
        s.vaddr = e->vaddr;
        snprintf(s.raw_name, sizeof s.raw_name, "%s", e->name);
        snprintf(s.module,   sizeof s.module,   "%s", mod);
        build_calynda_name(&d, e->name, mod, s.name, sizeof s.name);
        build_signature(&d, e->name, s.cal_sig, sizeof s.cal_sig,
                        &s.is_variadic);
        push(out, s);
    }

    /* 6.1b — scan every bl / bla whose target is outside any mapped
     *        text section.  These are "true" dynamic externs. */
    for (size_t i = 0; i < img->section_count; i++) {
        const DolSection *ds = &img->sections[i];
        if (ds->kind != DOL_SECTION_TEXT) continue;
        const uint8_t *data = img->bytes + ds->file_offset;
        for (uint32_t off = 0; off < ds->size; off += 4u) {
            uint32_t w = ((uint32_t)data[off] << 24) |
                         ((uint32_t)data[off+1] << 16) |
                         ((uint32_t)data[off+2] <<  8) |
                          (uint32_t)data[off+3];
            PpcInsn in;
            ppc_decode(w, ds->virtual_addr + off, &in);
            if (in.op != PPC_OP_BL && in.op != PPC_OP_BLA) continue;
            if (in_text(img, in.target)) continue;
            if (already_have(out, in.target)) continue;
            ExternSymbol s = { 0 };
            s.vaddr = in.target;
            s.is_unmapped = 1;
            snprintf(s.raw_name, sizeof s.raw_name,
                     "sub_%08x", in.target);
            snprintf(s.module, sizeof s.module, "unmapped");
            snprintf(s.name, sizeof s.name, "sub_%08x", in.target);
            snprintf(s.cal_sig, sizeof s.cal_sig, "fn(...args) -> ()");
            s.is_variadic = 1;
            push(out, s);
        }
    }
    return 0;
}

void externs_free(Externs *ex) {
    free(ex->entries);
    memset(ex, 0, sizeof *ex);
}

void externs_print(const Externs *ex, FILE *out) {
    size_t per[64] = { 0 };
    const char *names[64] = { 0 };
    size_t nmods = 0;
    for (size_t i = 0; i < ex->count; i++) {
        const char *m = ex->entries[i].module;
        size_t j;
        for (j = 0; j < nmods; j++)
            if (strcmp(names[j], m) == 0) { per[j]++; break; }
        if (j == nmods && nmods < 64) { names[nmods] = m; per[nmods++] = 1; }
    }
    fprintf(out, "# Externs: %zu symbols across %zu modules\n",
            ex->count, nmods);
    for (size_t j = 0; j < nmods; j++)
        fprintf(out, "  %-12s %zu\n", names[j], per[j]);
    for (size_t i = 0; i < ex->count; i++) {
        const ExternSymbol *s = &ex->entries[i];
        fprintf(out, "0x%08x  [%s]  extern fn %s %s%s\n",
                s->vaddr, s->module[0] ? s->module : "?",
                s->name, s->cal_sig + 2 /* skip leading "fn" */,
                s->is_variadic ? "  # variadic" : "");
    }
}
