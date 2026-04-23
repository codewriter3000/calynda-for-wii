/*
 * Shared internals for the SSA module. Private to src/cli/ssa/.
 */
#ifndef CALYNDA_DECOMPILER_SRC_CLI_SSA_INTERNAL_H
#define CALYNDA_DECOMPILER_SRC_CLI_SSA_INTERNAL_H

#include "cli/ssa.h"

#include <stddef.h>
#include <stdint.h>

/* Append to the string pool and return a stable pointer. Returns NULL
 * on OOM; the pool never shrinks so earlier pointers stay valid. */
const char *ssa_intern(Ssa *ssa, const char *s);
const char *ssa_internf(Ssa *ssa, const char *fmt, ...);

/* Pass entry points (one per file). Each returns 0 on success. */
int ssa_compute_defuse(Ssa *ssa);        /* 2.1 */
int ssa_place_phis(Ssa *ssa);            /* 2.2 */
int ssa_fold_prologue(Ssa *ssa);         /* 2.3 */
int ssa_build_expressions(Ssa *ssa);     /* 2.4 */
int ssa_map_stack_slots(Ssa *ssa);       /* 2.5 */
int ssa_reconstruct_calls(Ssa *ssa);     /* 2.6 */

/* Shared helpers. */
int      ssa_block_of_pc(const Ssa *ssa, uint32_t pc, uint32_t *out);
unsigned ssa_popcount64(uint64_t v);

#endif
