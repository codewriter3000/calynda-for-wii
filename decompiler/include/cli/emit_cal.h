/*
 * Calynda .cal emitter (requirement 8).
 *
 *   8.1 Package + import header — from recovered namespaces and the
 *       dependency graph of bl targets.
 *   8.2 extern declarations — one per SDK reference, reusing the
 *       signatures from requirement 6 (cli/externs.h).
 *   8.3 Top-level bindings — globals from data sections emitted as
 *       `var` / `final` with literal initializers where possible.
 *   8.4 Function body — walks the CFG + SSA expression trees and
 *       emits Calynda bodies (start()-style or lambda-style).
 *   8.5 Control-flow statements — maps CFG regions (linear / if / if-
 *       else / loop) onto Calynda ternary, tail-recursive helpers,
 *       and explicit returns; `discard _` for void calls.
 *   8.6 Formatting + splitting — enforces the repo's 250-line cap
 *       per .cal file by spilling overflow into `_partN.cal` siblings
 *       and by writing one file per top-level namespace component.
 *
 * The emitter is intentionally dependency-light: it consumes only
 * DolImage + SymTable + Externs (all already built by earlier
 * requirements) and walks CFG / SSA on demand per function.
 */
#ifndef CALYNDA_DECOMPILER_CLI_EMIT_CAL_H
#define CALYNDA_DECOMPILER_CLI_EMIT_CAL_H

#include "cli/externs.h"
#include "cli/symbols.h"
#include "dol.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#define EMIT_CAL_LINE_LIMIT 250

typedef struct {
    const DolImage *img;
    const SymTable *syms;
    const Externs  *externs;
    const char     *out_dir;         /* created if absent                */
    unsigned        line_limit;      /* defaults to 250 if zero          */
    unsigned        max_functions;   /* 0 = unlimited                    */
} EmitCalOpts;

typedef struct {
    unsigned modules_emitted;
    unsigned files_written;
    unsigned functions_emitted;
    unsigned externs_emitted;
    unsigned globals_emitted;
    unsigned split_siblings;         /* files exceeded line_limit        */
} EmitCalStats;

/* Primary entry point: emits a full source tree under opts->out_dir.
 * Returns 0 on success, non-zero on I/O error. */
int emit_cal(const EmitCalOpts *opts, EmitCalStats *stats_out);

#endif /* CALYNDA_DECOMPILER_CLI_EMIT_CAL_H */
