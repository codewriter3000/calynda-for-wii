/*
 * Top-level runners that drive the DOL/ISO pipeline and invoke the
 * analysis passes selected via CLI flags.
 */
#ifndef CALYNDA_DECOMPILER_CLI_RUN_H
#define CALYNDA_DECOMPILER_CLI_RUN_H

#include "cli/symbols.h"

#include <stdint.h>
#include <stdio.h>

typedef struct {
    unsigned     insns;
    int          all_after_entry;
    const char  *extract_dir;
    const char  *function_name;
    int          dump_data_flag;
    const char  *call_graph_path;
    const char  *xrefs_name;
    int          strings_flag;
    const char  *section_name;
    int          all_functions_flag;
    int          stats_flag;
    const char  *diff_path;
    const char  *cfg_name;
    const char  *ssa_name;
    const char  *types_name;
    int          classes;
    int          demangle;
    const char  *externs_dir;
    const char  *emit_cal_dir;
} RunOptions;

int run_on_dol_file(const char *path, const RunOptions *opts,
                    const SymTable *syms, FILE *out);

int run_on_iso_file(const char *path, const RunOptions *opts,
                    const char *key_path, const char *key_hex,
                    int partition_arg,
                    const SymTable *syms, FILE *out);

#endif /* CALYNDA_DECOMPILER_CLI_RUN_H */
