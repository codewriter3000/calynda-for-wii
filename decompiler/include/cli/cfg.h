/*
 * Control-flow graph for a single decoded PowerPC function.
 *
 * Given a flat sequence of instructions covering one function's body,
 * cfg_build() produces:
 *   - a basic-block split (new block at every branch target and after
 *     every branch / blr / bctr / return);
 *   - a successor edge list with kind annotations (fall-through,
 *     unconditional, conditional-taken, conditional-not-taken, switch
 *     case, return, indirect-unknown);
 *   - dominance information (immediate dominator per block) via the
 *     Cooper-Harvey-Kennedy iterative algorithm;
 *   - loop classification (back-edges, loop headers, loop depth);
 *   - if/else regions for conditional branches whose arms reconverge;
 *   - a switch-table recovery for the jump-table + bctr idiom;
 *   - an irreducibility flag when back-edges violate dominance.
 *
 * This module operates on a contiguous range of the DOL image. The
 * caller supplies the image plus the [start, end) virtual-address
 * range of a single function (usually via the ogws symbol table).
 * The CFG owns no instruction bytes; it only stores indices and vaddrs.
 */
#ifndef CALYNDA_DECOMPILER_CLI_CFG_H
#define CALYNDA_DECOMPILER_CLI_CFG_H

#include "dol.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#define CFG_INVALID 0xFFFFFFFFu

typedef enum {
    CFG_EDGE_FALLTHROUGH = 0,  /* straight-line next block                 */
    CFG_EDGE_UNCOND,           /* b / bl (non-tail) unconditional          */
    CFG_EDGE_COND_TAKEN,       /* bc when condition holds                  */
    CFG_EDGE_COND_NOT_TAKEN,   /* bc fall-through (condition fails)        */
    CFG_EDGE_SWITCH_CASE,      /* one arm of a recovered jump table        */
    CFG_EDGE_SWITCH_DEFAULT,   /* bounds-check fall-through                */
    CFG_EDGE_INDIRECT_UNKNOWN, /* bctr we couldn't resolve to a table      */
    CFG_EDGE_RETURN            /* blr / tail call leaves the function      */
} CfgEdgeKind;

typedef enum {
    CFG_REGION_LINEAR = 0,
    CFG_REGION_IF,             /* if without else                          */
    CFG_REGION_IF_ELSE,        /* if / else both reconverge                */
    CFG_REGION_LOOP,           /* header of a natural loop                 */
    CFG_REGION_SWITCH,         /* block ending in recovered jump table     */
    CFG_REGION_UNSTRUCTURED    /* irreducible fallback: treat as goto      */
} CfgRegionKind;

typedef struct {
    uint32_t from;         /* block index                                  */
    uint32_t to;           /* block index, or CFG_INVALID for RETURN/UNK   */
    CfgEdgeKind kind;
    int32_t  case_value;   /* switch case index, -1 otherwise              */
} CfgEdge;

typedef struct {
    uint32_t start_vaddr;  /* first instruction in block                   */
    uint32_t end_vaddr;    /* vaddr of one past last instruction           */
    unsigned insn_count;

    /* Populated by cfg_build. */
    uint32_t idom;                 /* immediate dominator index            */
    uint8_t  is_loop_header;       /* some edge back-branches here         */
    uint8_t  is_loop_exit;         /* has a successor outside its loop     */
    uint16_t loop_depth;

    /* Populated by region analysis. */
    CfgRegionKind region_kind;
    uint32_t      if_merge;        /* merge block for IF / IF_ELSE         */
} CfgBlock;

typedef struct {
    uint32_t case_value;
    uint32_t target_vaddr;
} CfgSwitchCase;

typedef struct {
    uint32_t block_index;          /* block ending with the bctr           */
    uint32_t table_vaddr;          /* .data address holding case pointers  */
    uint32_t default_vaddr;        /* bounds-check fallthrough target      */
    CfgSwitchCase *cases;
    size_t         case_count;
} CfgSwitch;

typedef struct {
    CfgBlock  *blocks;
    size_t     block_count;

    CfgEdge   *edges;
    size_t     edge_count;

    CfgSwitch *switches;
    size_t     switch_count;

    uint32_t   entry_block;        /* block containing start_vaddr         */
    uint8_t    is_irreducible;     /* back-edge head does not dominate tail*/
} Cfg;

/*
 * Build a CFG for the contiguous range [start, end) inside `img`.
 * Instruction bytes are read directly from the image. The caller owns
 * `img` and must keep it alive while `out` is used.
 *
 * Returns 0 on success. On error, `out` is left zeroed and a diagnostic
 * is written to stderr.
 */
int cfg_build(const DolImage *img,
              uint32_t start_vaddr, uint32_t end_vaddr,
              Cfg *out);

void cfg_free(Cfg *cfg);

/* Write a human-readable summary of the CFG to `out`. */
void cfg_print(const Cfg *cfg, FILE *out);

#endif /* CALYNDA_DECOMPILER_CLI_CFG_H */
