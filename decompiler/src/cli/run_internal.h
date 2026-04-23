/* Internal helpers shared between run.c and run_iso.c. Not exposed. */
#ifndef CALYNDA_DECOMPILER_CLI_RUN_INTERNAL_H
#define CALYNDA_DECOMPILER_CLI_RUN_INTERNAL_H

#include "cli/run.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

int disassemble_dol(const uint8_t *bytes, size_t n,
                    const RunOptions *opts,
                    const SymTable *syms, FILE *out);

long    run_file_size(FILE *fp);
uint8_t *run_slurp_file(const char *path, size_t *n_out);

#endif
