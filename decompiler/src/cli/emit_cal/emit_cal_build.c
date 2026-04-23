/*
 * Orchestrator: `emit_cal()` produces a full .cal source tree.
 *
 * Layout:
 *   <out>/main.cal              — package main, entrypoint wrapper,
 *                                 global data bindings, and every
 *                                 symbol that didn't map to a C++
 *                                 namespace.
 *   <out>/<ns>.cal              — one file per top-level namespace
 *                                 component (e.g. nw4r.cal, egg.cal),
 *                                 holding the functions in that ns.
 *   <out>/rvl_<module>.cal      — externs, delegated to the existing
 *                                 req-6 emitter (externs_emit_dir).
 *
 * When any file crosses the configured line limit (default 250) it
 * rotates transparently into `<basename>_part2.cal`, `_part3`, ...
 */
#include "cli/emit_cal.h"
#include "emit_cal_internal.h"

#include <stdlib.h>
#include <string.h>

#define MAX_PACKAGES 64

typedef struct {
    char     name[64];         /* basename: "main", "nw4r", ...       */
    EmitFile file;
    unsigned function_count;
} PkgFile;

static PkgFile *find_or_open(PkgFile *pkgs, size_t *n_pkgs,
                             const char *dir, const char *name,
                             unsigned limit) {
    for (size_t i = 0; i < *n_pkgs; i++)
        if (strcmp(pkgs[i].name, name) == 0) return &pkgs[i];
    if (*n_pkgs >= MAX_PACKAGES) return NULL;
    PkgFile *p = &pkgs[*n_pkgs];
    snprintf(p->name, sizeof p->name, "%s", name);
    if (emit_file_open(&p->file, dir, name, limit) != 0) return NULL;
    (*n_pkgs)++;
    return p;
}

static int in_text(const DolImage *img, uint32_t vaddr) {
    const DolSection *s = dol_section_for_vaddr(img, vaddr);
    return s && s->kind == DOL_SECTION_TEXT;
}

/* Build the `import` list for a package file. Every package imports
 * `rvl.*` so extern references resolve. */
static const char *const kMainImports[] = {
    "rvl.os",
    "rvl.gx",
    "rvl.pad",
    "rvl.wpad",
    "rvl.ax",
    "rvl.dvd",
    "rvl.vi",
    "rvl.msl",
};

int emit_cal(const EmitCalOpts *opts, EmitCalStats *out) {
    EmitCalStats stats = {0};
    if (!opts) { fprintf(stderr, "emit_cal: null opts\n"); return -1; }
    if (!opts->out_dir) { fprintf(stderr, "emit_cal: null out_dir\n"); return -1; }
    if (!opts->img) { fprintf(stderr, "emit_cal: null img\n"); return -1; }

    unsigned limit = opts->line_limit ? opts->line_limit : EMIT_CAL_LINE_LIMIT;

    /* 8.2: delegate extern emission to the req-6 emitter. It already
     * produces rvl_<module>.cal with the correct signatures. */
    if (opts->externs) {
        int files = externs_emit_dir(opts->externs, opts->out_dir);
        if (files > 0) {
            stats.externs_emitted = (unsigned)opts->externs->count;
            stats.files_written  += (unsigned)files;
        }
    }

    /* Per-package file set. Opened lazily. */
    PkgFile pkgs[MAX_PACKAGES] = {0};
    size_t  n_pkgs = 0;

    /* Always open main.cal first so globals + entrypoint land there. */
    PkgFile *main_pkg = find_or_open(pkgs, &n_pkgs, opts->out_dir, "main",
                                     limit);
    if (!main_pkg) { fprintf(stderr, "emit_cal: cannot open main.cal in %s\n", opts->out_dir); return -1; }

    /* 8.1: package + imports for main. Per-namespace package files
     * get their own header when we first touch them below. */
    emit_package_header(&main_pkg->file, "main",
                        kMainImports,
                        sizeof kMainImports / sizeof kMainImports[0]);

    /* 8.3: emit globals into main.cal. */
    int n_glob = emit_globals(&main_pkg->file, opts->img, opts->syms);
    if (n_glob > 0) stats.globals_emitted = (unsigned)n_glob;

    /* Entrypoint wrapper (boot/start). */
    emit_linef(&main_pkg->file, "# --- program entrypoint ---");
    emit_linef(&main_pkg->file,
               "# DOL entry is at 0x%08x; wire it into Calynda's start().",
               opts->img->entrypoint);
    emit_linef(&main_pkg->file, "start(args: arr<string>) -> {");
    emit_linef(&main_pkg->file,
               "    # TODO: reimplement entrypoint 0x%08x in Calynda",
               opts->img->entrypoint);
    emit_linef(&main_pkg->file, "    return 0");
    emit_linef(&main_pkg->file, "};");
    emit_linef(&main_pkg->file, "");

    /* 8.4 / 8.5: emit each text-section symbol into its package file. */
    if (opts->syms) {
        for (size_t i = 0; i < opts->syms->count; i++) {
            const SymEntry *e = &opts->syms->entries[i];
            if (!in_text(opts->img, e->vaddr)) continue;
            if (e->size == 0) continue;
            if (opts->max_functions &&
                stats.functions_emitted >= opts->max_functions) break;

            char pkg_name[64];
            emit_symbol_package(e->name, pkg_name, sizeof pkg_name);

            PkgFile *pkg = find_or_open(pkgs, &n_pkgs, opts->out_dir,
                                        pkg_name, limit);
            if (!pkg) continue;
            if (pkg->function_count == 0 && strcmp(pkg_name, "main") != 0) {
                /* First touch of a namespace file: write its header. */
                emit_package_header(&pkg->file, pkg_name,
                                    kMainImports,
                                    sizeof kMainImports /
                                        sizeof kMainImports[0]);
            }
            int rc = emit_function(&pkg->file, opts->img, opts->syms,
                                   opts->externs, e);
            if (rc > 0) {
                pkg->function_count++;
                stats.functions_emitted++;
            }
        }
    }

    /* Roll everything up. */
    for (size_t i = 0; i < n_pkgs; i++) {
        stats.files_written    += pkgs[i].file.total_files;
        stats.split_siblings   += pkgs[i].file.split_siblings;
        emit_file_close(&pkgs[i].file);
    }
    stats.modules_emitted = (unsigned)n_pkgs;

    if (out) *out = stats;
    return 0;
}
