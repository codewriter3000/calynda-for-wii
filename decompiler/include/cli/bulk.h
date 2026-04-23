/*
 * Bulk-function analyses:
 *   - emit_all_functions(): write every sized symbol that maps into a
 *     text section as a labelled block into a single output stream.
 *   - emit_stats(): report text coverage, unknown-instruction counts,
 *     and the largest gaps between consecutive sized symbols.
 *   - emit_diff(): byte-by-byte compare every sized symbol between two
 *     DOL images and print the first mismatch in each.
 */
#ifndef CALYNDA_DECOMPILER_CLI_BULK_H
#define CALYNDA_DECOMPILER_CLI_BULK_H

#include "cli/symbols.h"
#include "dol.h"

#include <stdio.h>

int emit_all_functions(const DolImage *img, const SymTable *syms, FILE *out);
int emit_stats(const DolImage *img, const SymTable *syms, FILE *out);
int emit_diff(const DolImage *a, const DolImage *b,
              const SymTable *syms, FILE *out);

#endif
