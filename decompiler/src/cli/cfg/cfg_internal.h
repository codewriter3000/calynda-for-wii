/*
 * CFG internals shared between cfg_build.c, cfg_dominance.c,
 * cfg_regions.c, cfg_switch.c, and cfg_print.c.
 *
 * Not installed; only visible via -Isrc.
 */
#ifndef CALYNDA_DECOMPILER_SRC_CLI_CFG_INTERNAL_H
#define CALYNDA_DECOMPILER_SRC_CLI_CFG_INTERNAL_H

#include "cli/cfg.h"
#include "dol.h"
#include "ppc.h"

#include <stddef.h>
#include <stdint.h>

/* Read the 32-bit big-endian word at `vaddr` inside `img`. Returns 0
 * and sets *ok=1 on success, or returns 0 with *ok=0 when the address
 * is not mapped. */
uint32_t cfg_read_word(const DolImage *img, uint32_t vaddr, int *ok);

/* Return index of the block whose start_vaddr == `vaddr`, or
 * CFG_INVALID if no such block. Linear scan; CFGs are small. */
uint32_t cfg_block_of_vaddr(const Cfg *cfg, uint32_t vaddr);

/* Append a successor edge. Returns 0 on success, -1 on OOM. */
int cfg_add_edge(Cfg *cfg, uint32_t from, uint32_t to,
                 CfgEdgeKind kind, int32_t case_value);

/* Append a block. Returns index on success, CFG_INVALID on OOM. */
uint32_t cfg_add_block(Cfg *cfg, uint32_t start_vaddr);

/* ----------------------------------------------------------------- */
/* Internal helpers that each live in their own .c file.             */
/* ----------------------------------------------------------------- */

/* cfg_dominance.c */
int cfg_compute_dominance(Cfg *cfg);
int cfg_classify_loops(Cfg *cfg);

/* cfg_regions.c */
int cfg_classify_regions(Cfg *cfg);

/* cfg_switch.c — returns number of switches recognized, or -1 on error. */
int cfg_recover_switches(Cfg *cfg, const DolImage *img);

#endif
