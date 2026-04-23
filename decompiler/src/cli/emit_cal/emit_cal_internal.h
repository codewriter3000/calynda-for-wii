/* Private internals for the Calynda emitter.  */
#ifndef CALYNDA_DECOMPILER_CLI_EMIT_CAL_INTERNAL_H
#define CALYNDA_DECOMPILER_CLI_EMIT_CAL_INTERNAL_H

#include "cli/emit_cal.h"
#include "cli/cfg.h"
#include "cli/ssa.h"
#include "cli/types.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

/* A handle to a file under opts->out_dir that counts emitted lines
 * and transparently spills to `<basename>_part2.cal`, `_part3`, ...
 * when the caller crosses EMIT_CAL_LINE_LIMIT.  Every emit_* helper
 * below routes writes through an EmitFile. */
typedef struct EmitFile {
    const char *dir;
    char        basename[64];      /* "nw4r", "main", ...                */
    FILE       *fp;
    unsigned    lines;
    unsigned    part;              /* 1, 2, 3, ...                       */
    unsigned    limit;
    unsigned    split_siblings;    /* number of extra files used         */
    unsigned    total_files;       /* including the primary              */
} EmitFile;

int  emit_file_open (EmitFile *ef, const char *dir, const char *basename,
                     unsigned limit);
void emit_file_close(EmitFile *ef);

/* Printf-style writer. Counts every newline, rotates to a new sibling
 * once the line budget is exhausted. Rotation emits a `# continued ...`
 * pair so the spill points are self-describing. */
int  emit_linef(EmitFile *ef, const char *fmt, ...)
    __attribute__((format(printf, 2, 3)));
int  emit_blank(EmitFile *ef);

/* Subsection emitters (8.1–8.5). */
int emit_package_header(EmitFile *ef, const char *package,
                        const char *const *imports, size_t n_imports);
int emit_extern_block(EmitFile *ef, const Externs *ex, const char *module);
int emit_globals(EmitFile *ef, const DolImage *img,
                 const SymTable *syms);
int emit_function(EmitFile *ef, const DolImage *img,
                  const SymTable *syms, const Externs *ex,
                  const SymEntry *fn);

/* Little helper: produce a Calynda-safe identifier from a raw symbol. */
void emit_safe_ident(const char *raw, char *out, size_t n);

/* Classify a symbol's target file.
 *   - Mangled C++ symbols in `nw4r::`, `EGG::`, `JSystem::`, ...
 *     go into a file named after the top package component.
 *   - Everything else lands in `main.cal`.
 * Returns the basename (no extension). */
void emit_symbol_package(const char *raw, char *out, size_t n);

#endif /* CALYNDA_DECOMPILER_CLI_EMIT_CAL_INTERNAL_H */
