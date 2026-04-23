/*
 * CodeWarrior name demangler — main entry (requirement 5.2 / 5.4 / 5.5).
 */
#include "cli/demangle.h"
#include "demangle_internal.h"
#include <stdio.h>
#include <string.h>

/* Locate the `__` that separates the method name from the qualifier /
 * signature.  The separator is only valid when followed by `F`, `Q<n>`
 * or a length-prefixed name.  Leading `__` (e.g. `__ct`, `__pl`) are
 * skipped over so that they count as part of the method name. */
static const char *find_split(const char *s) {
    size_t start = (s[0] == '_' && s[1] == '_') ? 2 : 0;
    for (size_t i = start; s[i]; i++) {
        if (s[i] == '_' && s[i + 1] == '_') {
            const char *p = s + i + 2;
            if (*p == 'F' || *p == 'Q' ||
                (*p >= '1' && *p <= '9')) return s + i;
        }
    }
    return NULL;
}

static void last_component(const char *qual, char *out, size_t n) {
    const char *c = strrchr(qual, ':');
    snprintf(out, n, "%s", c ? c + 1 : qual);
}

int cw_demangle(const char *mangled, CwDemangled *out) {
    memset(out, 0, sizeof *out);
    snprintf(out->pretty, sizeof out->pretty, "%s", mangled);
    if (!mangled || !*mangled) return 0;

    const char *sep = find_split(mangled);
    if (!sep) return 0;

    size_t name_len = (size_t)(sep - mangled);
    char name[128];
    if (name_len == 0 || name_len >= sizeof name) return 0;
    memcpy(name, mangled, name_len);
    name[name_len] = 0;

    const char *rest = sep + 2;

    if (*rest == 'Q' || (*rest >= '1' && *rest <= '9')) {
        const char *p = rest;
        if (!dmg_parse_type(&p, out->qualifier, sizeof out->qualifier))
            return 0;
        rest = p;
    }

    /* Optional const / volatile member-function markers: `C`, `V`, `CV`. */
    int is_const = 0, is_volatile = 0;
    while (*rest == 'C' || *rest == 'V') {
        if (*rest == 'C') is_const = 1;
        if (*rest == 'V') is_volatile = 1;
        rest++;
    }

    if (*rest != 'F') return 0;
    rest++;

    char args[384]; args[0] = 0;
    while (*rest && *rest != '_') {
        char t[128];
        if (!dmg_parse_type(&rest, t, sizeof t)) break;
        if (args[0]) strncat(args, ", ", sizeof args - strlen(args) - 1);
        strncat(args, t, sizeof args - strlen(args) - 1);
    }
    if (!args[0]) snprintf(args, sizeof args, "void");
    snprintf(out->args, sizeof out->args, "(%s)", args);

    /* Classify method name (5.4 / 5.5). */
    if (strcmp(name, "__ct") == 0 && out->qualifier[0]) {
        out->kind = CW_KIND_CTOR;
        last_component(out->qualifier, out->method, sizeof out->method);
    } else if (strcmp(name, "__dt") == 0 && out->qualifier[0]) {
        out->kind = CW_KIND_DTOR;
        char base[96];
        last_component(out->qualifier, base, sizeof base);
        snprintf(out->method, sizeof out->method, "~%s", base);
    } else if (name[0] == '_' && name[1] == '_' && name[2]) {
        const char *op = cw_operator(name + 2);
        if (op) {
            out->kind = CW_KIND_OPERATOR;
            snprintf(out->method, sizeof out->method, "operator%s", op);
        } else {
            snprintf(out->method, sizeof out->method, "%s", name);
        }
    } else {
        snprintf(out->method, sizeof out->method, "%s", name);
    }

    if (!out->qualifier[0] && out->kind == CW_KIND_REGULAR)
        out->kind = CW_KIND_FREE;

    if (out->qualifier[0])
        snprintf(out->pretty, sizeof out->pretty, "%s::%s%s%s%s",
                 out->qualifier, out->method, out->args,
                 is_const ? " const" : "",
                 is_volatile ? " volatile" : "");
    else
        snprintf(out->pretty, sizeof out->pretty, "%s%s%s%s",
                 out->method, out->args,
                 is_const ? " const" : "",
                 is_volatile ? " volatile" : "");

    out->ok = 1;
    return 1;
}
