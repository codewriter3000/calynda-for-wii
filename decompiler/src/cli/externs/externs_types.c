/*
 * C++ type string -> Calynda type string (requirement 6.3 / 6.4).
 *
 * Input is produced by the demangler, e.g.
 *     "unsigned int"
 *     "const DVDFileInfo*"
 *     "void(*)(int, float)"
 * Output uses Calynda-ish primitives:
 *     u32, *DVDFileInfo, fn(i32, f32) -> ()
 */
#include "cli/externs.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>

static int starts_with(const char *s, const char *p) {
    return strncmp(s, p, strlen(p)) == 0;
}

/* Map a bare primitive token sequence to a Calynda primitive.  Returns
 * 1 on success.  Consumes `*s` up to the matched token. */
static int match_primitive(const char *s, const char **end, char *out, size_t n) {
    const struct { const char *cpp; const char *cal; } kPrims[] = {
        { "unsigned long long", "u64" },
        { "signed long long",   "i64" },
        { "unsigned long",      "u32" },
        { "signed long",        "i32" },
        { "unsigned int",       "u32" },
        { "signed int",         "i32" },
        { "unsigned short",     "u16" },
        { "signed short",       "i16" },
        { "unsigned char",      "u8"  },
        { "signed char",        "i8"  },
        { "long long",          "i64" },
        { "wchar_t",            "u16" },
        { "double",             "f64" },
        { "float",              "f32" },
        { "bool",               "bool"},
        { "void",               "()"  },
        { "long",               "i32" },
        { "int",                "i32" },
        { "short",              "i16" },
        { "char",               "i8"  },
    };
    for (size_t i = 0; i < sizeof kPrims / sizeof kPrims[0]; i++) {
        if (starts_with(s, kPrims[i].cpp)) {
            snprintf(out, n, "%s", kPrims[i].cal);
            *end = s + strlen(kPrims[i].cpp);
            return 1;
        }
    }
    return 0;
}

static void trim(char *s) {
    size_t n = strlen(s);
    while (n && isspace((unsigned char)s[n - 1])) s[--n] = 0;
    size_t i = 0;
    while (s[i] && isspace((unsigned char)s[i])) i++;
    if (i) memmove(s, s + i, strlen(s + i) + 1);
}

static void convert_one(const char *cpp, char *out, size_t n);

/* Handle function-pointer syntax `R(*)(A, B)`. */
static int match_fnptr(const char *cpp, char *out, size_t n) {
    const char *paren = strstr(cpp, "(*)(");
    if (!paren) return 0;
    /* ret_type is everything before `(*)(`. */
    char ret[128];
    size_t rl = (size_t)(paren - cpp);
    if (rl >= sizeof ret) rl = sizeof ret - 1;
    memcpy(ret, cpp, rl); ret[rl] = 0;
    trim(ret);
    char rcal[128]; convert_one(ret, rcal, sizeof rcal);

    const char *args_start = paren + 4;
    const char *args_end = strrchr(args_start, ')');
    if (!args_end) return 0;
    size_t al = (size_t)(args_end - args_start);
    char args[256];
    if (al >= sizeof args) al = sizeof args - 1;
    memcpy(args, args_start, al); args[al] = 0;

    /* Split args by top-level commas. */
    char cal_args[256] = "";
    int depth = 0;
    const char *seg = args;
    for (const char *p = args; ; p++) {
        if (*p == '(' || *p == '<') depth++;
        else if (*p == ')' || *p == '>') depth--;
        if ((*p == ',' && depth == 0) || *p == 0) {
            size_t len = (size_t)(p - seg);
            char one[128];
            if (len >= sizeof one) len = sizeof one - 1;
            memcpy(one, seg, len); one[len] = 0;
            trim(one);
            if (one[0]) {
                char cone[128]; convert_one(one, cone, sizeof cone);
                if (cal_args[0])
                    strncat(cal_args, ", ", sizeof cal_args - strlen(cal_args) - 1);
                strncat(cal_args, cone, sizeof cal_args - strlen(cal_args) - 1);
            }
            if (!*p) break;
            seg = p + 1;
        }
    }
    snprintf(out, n, "fn(%s) -> %s", cal_args, rcal);
    return 1;
}

static void convert_one(const char *cpp_in, char *out, size_t n) {
    if (!n) return;
    char s[256];
    snprintf(s, sizeof s, "%s", cpp_in);
    trim(s);
    if (!*s) { out[0] = 0; return; }

    if (strcmp(s, "...") == 0) { snprintf(out, n, "..."); return; }
    if (match_fnptr(s, out, n)) return;

    /* Count trailing pointer / reference markers. */
    int ptrs = 0;
    size_t len = strlen(s);
    while (len && (s[len - 1] == '*' || s[len - 1] == '&')) {
        ptrs++;
        s[--len] = 0;
    }
    trim(s);

    /* Strip leading `const ` / `volatile `. */
    while (starts_with(s, "const ") || starts_with(s, "volatile ")) {
        size_t k = starts_with(s, "const ") ? 6 : 9;
        memmove(s, s + k, strlen(s + k) + 1);
    }
    trim(s);

    char base[128];
    const char *end = NULL;
    if (match_primitive(s, &end, base, sizeof base)) {
        /* Ignore trailing junk after the primitive. */
    } else {
        /* Non-primitive: use the raw identifier, collapse `::` -> `.`,
         * and strip template parameters. */
        size_t bi = 0;
        for (size_t i = 0; s[i] && bi + 2 < sizeof base; i++) {
            if (s[i] == ':' && s[i + 1] == ':') { base[bi++] = '.'; i++; }
            else if (s[i] == '<') {
                /* Skip until matching '>'. */
                int d = 1;
                while (s[i + 1] && d) {
                    i++;
                    if (s[i] == '<') d++;
                    else if (s[i] == '>') d--;
                }
            } else if (!isspace((unsigned char)s[i])) {
                base[bi++] = s[i];
            }
        }
        base[bi] = 0;
        if (!base[0]) snprintf(base, sizeof base, "unknown");
    }

    char stars[8] = "";
    for (int i = 0; i < ptrs && i < 7; i++) stars[i] = '*';
    stars[ptrs < 7 ? ptrs : 7] = 0;
    snprintf(out, n, "%s%s", stars, base);
}

void externs_cpp_type_to_calynda(const char *cpp, char *out, size_t n) {
    convert_one(cpp, out, n);
}
