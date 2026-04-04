#include "c_emit_internal.h"

#include <stdio.h>
#include <string.h>

/* C keywords that must be prefixed to avoid collisions. */
static const char *C_KEYWORDS[] = {
    "auto", "break", "case", "char", "const", "continue", "default", "do",
    "double", "else", "enum", "extern", "float", "for", "goto", "if",
    "inline", "int", "long", "register", "restrict", "return", "short",
    "signed", "sizeof", "static", "struct", "switch", "typedef", "union",
    "unsigned", "void", "volatile", "while",
    /* C11 additions */
    "_Alignas", "_Alignof", "_Atomic", "_Bool", "_Complex", "_Generic",
    "_Imaginary", "_Noreturn", "_Static_assert", "_Thread_local",
    NULL
};

static bool is_c_keyword(const char *name) {
    const char **kw;

    for (kw = C_KEYWORDS; *kw; kw++) {
        if (strcmp(name, *kw) == 0) {
            return true;
        }
    }
    return false;
}

/* Emit a name, replacing '.' with '_'. */
void c_emit_mangle(FILE *out, const char *qualified_name) {
    const char *p;

    if (!qualified_name) {
        return;
    }
    for (p = qualified_name; *p; p++) {
        fputc(*p == '.' ? '_' : *p, out);
    }
}

/* Emit a simple identifier, prefixing C keywords. */
void c_emit_safe_name(FILE *out, const char *name) {
    if (!name) {
        return;
    }
    if (is_c_keyword(name)) {
        fprintf(out, "__cal_%s", name);
    } else {
        fputs(name, out);
    }
}

/* Emit the C variable/function name for a top-level or local binding. */
void c_emit_global_name(FILE *out, const char *name) {
    if (!name) {
        return;
    }
    fprintf(out, "__cal_");
    c_emit_mangle(out, name);
}

void c_emit_lambda_name(FILE *out, int id) {
    fprintf(out, "__calynda_lambda_%d", id);
}

void c_emit_closure_struct_name(FILE *out, int id) {
    fprintf(out, "__calynda_closure_%d_t", id);
}
