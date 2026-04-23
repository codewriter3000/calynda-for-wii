/*
 * Per-module .cal header emitter (requirement 6.5).
 */
#include "cli/externs.h"
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

static int ensure_dir(const char *dir) {
    struct stat st;
    if (stat(dir, &st) == 0) return S_ISDIR(st.st_mode) ? 0 : -1;
    if (errno != ENOENT) return -1;
    return mkdir(dir, 0755);
}

static int name_seen(char seen[][SYM_NAME_MAX], size_t n,
                     const char *name) {
    for (size_t i = 0; i < n; i++)
        if (strcmp(seen[i], name) == 0) return 1;
    return 0;
}

int externs_emit_dir(const Externs *ex, const char *dir) {
    if (ensure_dir(dir) != 0) return -1;

    char modules[64][EX_MODULE_MAX] = { { 0 } };
    size_t nmod = 0;
    for (size_t i = 0; i < ex->count; i++) {
        const char *m = ex->entries[i].module;
        if (!*m) continue;
        size_t j;
        for (j = 0; j < nmod; j++)
            if (strcmp(modules[j], m) == 0) break;
        if (j == nmod && nmod < 64)
            snprintf(modules[nmod++], EX_MODULE_MAX, "%s", m);
    }

    int files = 0;
    for (size_t j = 0; j < nmod; j++) {
        char path[512];
        snprintf(path, sizeof path, "%s/rvl_%s.cal", dir, modules[j]);
        FILE *f = fopen(path, "w");
        if (!f) return -1;
        fprintf(f, "# Auto-generated RVL SDK extern bindings (%s).\n",
                modules[j]);
        fprintf(f, "package rvl.%s\n\n", modules[j]);

        char seen[2048][SYM_NAME_MAX];
        size_t nseen = 0;
        int emitted = 0;
        for (size_t i = 0; i < ex->count; i++) {
            const ExternSymbol *s = &ex->entries[i];
            if (strcmp(s->module, modules[j]) != 0) continue;
            const char *n = s->name[0] ? s->name : s->raw_name;
            if (nseen < 2048 && name_seen(seen, nseen, n)) continue;
            if (nseen < 2048) snprintf(seen[nseen++], SYM_NAME_MAX, "%s", n);

            fprintf(f, "# 0x%08x  %s%s\n",
                    s->vaddr, s->raw_name,
                    s->is_variadic ? "  (variadic)" : "");
            fprintf(f, "extern %s %s\n\n", n, s->cal_sig);
            emitted++;
        }
        fclose(f);
        if (emitted) files++;
    }
    return files;
}
