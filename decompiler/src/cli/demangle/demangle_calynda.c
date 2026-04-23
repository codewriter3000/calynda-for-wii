/*
 * Calynda-specific mapping: namespaces -> packages, ctor/dtor/operator
 * bindings, and rename-table for uniqueness enforcement (req 5.3/5.4/
 * 5.5/5.6).
 */
#include "cli/demangle.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const struct { const char *cpp; const char *cal; } kNs[] = {
    { "EGG",     "egg" },
    { "nw4r",    "nw4r" },
    { "NW4R",    "nw4r" },
    { "JSystem", "jsystem" },
    { "JUtility","jutility" },
    { "JKernel", "jkernel" },
    { "JMath",   "jmath" },
    { "JGadget", "jgadget" },
    { "JSUtility","jsutility" },
};

static void lower_copy(const char *s, char *d, size_t n) {
    size_t i = 0;
    for (; s[i] && i + 1 < n; i++) d[i] = (char)tolower((unsigned char)s[i]);
    d[i] = 0;
}

static void pkg_for_head(const char *head, char *out, size_t n) {
    for (size_t i = 0; i < sizeof kNs / sizeof kNs[0]; i++)
        if (strcmp(kNs[i].cpp, head) == 0) {
            snprintf(out, n, "%s", kNs[i].cal);
            return;
        }
    lower_copy(head, out, n);
}

void cw_to_calynda_path(const char *qualifier, char *out, size_t n) {
    if (!n) return;
    out[0] = 0;
    if (!qualifier || !*qualifier) return;

    const char *p = qualifier;
    size_t used = 0;
    int first = 1;

    while (*p) {
        /* Skip any run of ':' separators. */
        while (*p == ':') p++;
        if (!*p) break;

        /* Read one component up to the next '::'. */
        const char *start = p;
        while (*p && *p != ':') p++;
        size_t clen = (size_t)(p - start);
        char comp[96];
        if (clen >= sizeof comp) clen = sizeof comp - 1;
        memcpy(comp, start, clen);
        comp[clen] = 0;

        char piece[96];
        if (first) pkg_for_head(comp, piece, sizeof piece);
        else       snprintf(piece, sizeof piece, "%s", comp);

        size_t plen = strlen(piece);
        size_t need = plen + (first ? 0 : 1);
        if (used + need + 1 >= n) break;
        if (!first) out[used++] = '.';
        memcpy(out + used, piece, plen);
        used += plen;
        out[used] = 0;
        first = 0;
    }
}

static const struct { const char *mn; const char *ident; } kOpIdent[] = {
    { "pl", "plus" },   { "mi", "minus" },  { "ml", "mul" },
    { "dv", "div" },    { "md", "mod" },    { "as", "assign" },
    { "eq", "eq" },     { "ne", "ne" },     { "lt", "lt" },
    { "gt", "gt" },     { "le", "le" },     { "ge", "ge" },
    { "ls", "shl" },    { "rs", "shr" },    { "or", "bor" },
    { "an", "band" },   { "er", "bxor" },   { "aa", "land" },
    { "oo", "lor" },    { "nt", "lnot" },   { "co", "bnot" },
    { "pp", "inc" },    { "mm", "dec" },    { "cl", "call" },
    { "vc", "index" },  { "rf", "arrow" },  { "rm", "arrow_star" },
    { "nw", "new" },    { "dl", "delete" }, { "apl", "add_assign" },
    { "ami", "sub_assign" }, { "aml", "mul_assign" },
    { "adv", "div_assign" }, { "ad", "addr" }, { "cm", "comma" },
};

static const char *op_ident(const char *method) {
    /* method looks like "operator+" etc. — map back via cw_operator
     * entries.  But we also have the raw method mnemonic when called
     * from cw_to_calynda_binding, so scan by glyph match. */
    if (strncmp(method, "operator", 8) != 0) return NULL;
    const char *g = method + 8;
    for (size_t i = 0; i < sizeof kOpIdent / sizeof kOpIdent[0]; i++) {
        const char *glyph = cw_operator(kOpIdent[i].mn);
        if (glyph && strcmp(glyph, g) == 0) return kOpIdent[i].ident;
    }
    return NULL;
}

void cw_to_calynda_binding(const CwDemangled *d, char *out, size_t n) {
    if (!n) return;
    out[0] = 0;
    char pkg[256]; cw_to_calynda_path(d->qualifier, pkg, sizeof pkg);
    const char *sep = pkg[0] ? "." : "";

    switch (d->kind) {
        case CW_KIND_CTOR:
            snprintf(out, n, "%s%snew", pkg, sep);
            return;
        case CW_KIND_DTOR:
            snprintf(out, n, "%s%sdrop", pkg, sep);
            return;
        case CW_KIND_OPERATOR: {
            const char *id = op_ident(d->method);
            snprintf(out, n, "%s%s%s", pkg, sep,
                     id ? id : d->method);
            return;
        }
        case CW_KIND_FREE:
            snprintf(out, n, "%s", d->method);
            return;
        case CW_KIND_REGULAR:
        default:
            snprintf(out, n, "%s%s%s", pkg, sep, d->method);
            return;
    }
}

/* --- 5.6 uniqueness enforcement ------------------------------------ */

typedef struct Entry {
    char *name;
    int count;
    struct Entry *next;
} Entry;

struct CwRenameTable { Entry *head; };

CwRenameTable *cw_rename_new(void) {
    return calloc(1, sizeof(CwRenameTable));
}

void cw_rename_free(CwRenameTable *t) {
    if (!t) return;
    Entry *e = t->head;
    while (e) { Entry *n = e->next; free(e->name); free(e); e = n; }
    free(t);
}

static Entry *find_entry(CwRenameTable *t, const char *name) {
    for (Entry *e = t->head; e; e = e->next)
        if (strcmp(e->name, name) == 0) return e;
    return NULL;
}

static char *dup_cstr(const char *s) {
    size_t n = strlen(s) + 1;
    char *d = malloc(n);
    if (d) memcpy(d, s, n);
    return d;
}

int cw_rename_unique(CwRenameTable *t, const char *base,
                     char *out, size_t n) {
    Entry *e = find_entry(t, base);
    if (!e) {
        e = calloc(1, sizeof *e);
        e->name = dup_cstr(base);
        e->count = 1;
        e->next = t->head;
        t->head = e;
        snprintf(out, n, "%s", base);
        return 0;
    }
    e->count++;
    snprintf(out, n, "%s_%d", base, e->count);
    return 1;
}
