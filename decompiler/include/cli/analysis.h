/*
 * Analysis passes that operate on a parsed DOL + symbol table:
 *   - hex+ASCII data dump with pointer annotations
 *   - printable-string scan
 *   - call-graph emission
 *   - per-function xref lookup
 *   - disassembly of a named text section
 *   - the underlying instruction-range printer
 */
#ifndef CALYNDA_DECOMPILER_CLI_ANALYSIS_H
#define CALYNDA_DECOMPILER_CLI_ANALYSIS_H

#include "cli/symbols.h"
#include "dol.h"

#include <stdint.h>
#include <stdio.h>

void emit_disassembly_range(FILE *out,
                            const uint8_t *ip,
                            uint32_t start_vaddr,
                            unsigned insn_count,
                            const SymTable *syms);

void dump_data(const DolImage *img, const SymTable *syms, FILE *out);
void dump_strings(const DolImage *img, const SymTable *syms, FILE *out);

int  emit_call_graph(const DolImage *img, const SymTable *syms, FILE *out);

int  emit_xrefs(const DolImage *img, const SymTable *syms,
                const char *target_name, FILE *out);

int  disassemble_section(const DolImage *img, const char *name,
                         const SymTable *syms, FILE *out);

#endif /* CALYNDA_DECOMPILER_CLI_ANALYSIS_H */
