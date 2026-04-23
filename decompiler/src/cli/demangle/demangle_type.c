/*
 * CodeWarrior type parser (requirement 5.1 / 5.2).
 *
 * Handles the grammar used by Metrowerks CodeWarrior for PowerPC:
 *
 *   primitive  ::= v|b|c|w|s|i|l|x|f|d|e
 *   qualifier  ::= Q <digit> {name}+
 *   name       ::= <digits> <that-many-chars>      (may contain < , >)
 *   type       ::= P type        (pointer)
 *               |  R type        (reference)
 *               |  C type        (const)
 *               |  V type        (volatile)
 *               |  U type        (unsigned)
 *               |  S type        (signed)
 *               |  A <digits> _ type   (array)
 *               |  F {type}* _ type    (function pointer)
 *               |  qualifier
 *               |  name
 *               |  primitive
 */
#include "cli/demangle.h"
#include "demangle_internal.h"
#include <stdio.h>
#include <string.h>

static int is_digit(char c) { return c >= '0' && c <= '9'; }

int dmg_parse_name(const char **pp, char *out, size_t n) {
    int len = 0;
    while (is_digit(**pp)) {
        len = len * 10 + (**pp - '0');
        (*pp)++;
    }
    if (len <= 0 || (size_t)len >= n) return 0;
    memcpy(out, *pp, (size_t)len);
    out[len] = 0;
    *pp += len;
    return 1;
}

int dmg_parse_type(const char **pp, char *out, size_t n) {
    if (n == 0 || !**pp) return 0;
    out[0] = 0;
    char c = **pp;

    if (c == 'P' || c == 'R') {
        (*pp)++;
        char inner[256];
        if (!dmg_parse_type(pp, inner, sizeof inner)) return 0;
        snprintf(out, n, "%s%c", inner, c == 'P' ? '*' : '&');
        return 1;
    }
    if (c == 'C' || c == 'V') {
        (*pp)++;
        char inner[256];
        if (!dmg_parse_type(pp, inner, sizeof inner)) return 0;
        snprintf(out, n, "%s %s", c == 'C' ? "const" : "volatile", inner);
        return 1;
    }
    if (c == 'U' || c == 'S') {
        (*pp)++;
        char inner[128];
        if (!dmg_parse_type(pp, inner, sizeof inner)) return 0;
        snprintf(out, n, "%s %s", c == 'U' ? "unsigned" : "signed", inner);
        return 1;
    }
    if (c == 'A') {
        (*pp)++;
        int dim = 0;
        while (is_digit(**pp)) { dim = dim * 10 + (**pp - '0'); (*pp)++; }
        if (**pp != '_') return 0;
        (*pp)++;
        char inner[128];
        if (!dmg_parse_type(pp, inner, sizeof inner)) return 0;
        snprintf(out, n, "%s[%d]", inner, dim);
        return 1;
    }
    if (c == 'F') {
        (*pp)++;
        char args[384]; args[0] = 0;
        while (**pp && **pp != '_') {
            char t[128];
            if (!dmg_parse_type(pp, t, sizeof t)) return 0;
            if (args[0])
                strncat(args, ", ", sizeof args - strlen(args) - 1);
            strncat(args, t, sizeof args - strlen(args) - 1);
        }
        char ret[128] = "void";
        if (**pp == '_') {
            (*pp)++;
            if (!dmg_parse_type(pp, ret, sizeof ret)) return 0;
        }
        snprintf(out, n, "%s(*)(%s)", ret, args);
        return 1;
    }
    if (c == 'Q') {
        (*pp)++;
        int cnt = **pp - '0';
        if (cnt < 1 || cnt > 9) return 0;
        (*pp)++;
        char qual[256]; qual[0] = 0;
        for (int i = 0; i < cnt; i++) {
            char comp[128];
            if (!dmg_parse_name(pp, comp, sizeof comp)) return 0;
            if (i) strncat(qual, "::", sizeof qual - strlen(qual) - 1);
            strncat(qual, comp, sizeof qual - strlen(qual) - 1);
        }
        snprintf(out, n, "%s", qual);
        return 1;
    }
    if (is_digit(c)) return dmg_parse_name(pp, out, n);

    const char *name = NULL;
    switch (c) {
        case 'v': name = "void";        break;
        case 'b': name = "bool";        break;
        case 'c': name = "char";        break;
        case 'w': name = "wchar_t";     break;
        case 's': name = "short";       break;
        case 'i': name = "int";         break;
        case 'l': name = "long";        break;
        case 'x': name = "long long";   break;
        case 'f': name = "float";       break;
        case 'd': name = "double";      break;
        case 'e': name = "...";         break;
    }
    if (!name) return 0;
    (*pp)++;
    snprintf(out, n, "%s", name);
    return 1;
}
