/*
 * Requirement 2.2 — place phi functions using dominance frontiers.
 *
 * Algorithm (Cytron et al.):
 *   DF[b] = { n | p in preds(n), b dominates p, b does not strictly
 *                 dominate n }
 * For each register r, the set of blocks needing a phi for r is the
 * iterated dominance frontier DF+(defs(r)). We further prune by
 * live-in: a phi is only useful where the register is live on entry.
 */
#include "ssa_internal.h"

#include <stdlib.h>
#include <string.h>

#define BIT(i) (1ULL << (unsigned)(i))

/* Walk from b1 up idom chain; return 1 if b2 is on that chain. */
static int dominates(const Cfg *cfg, uint32_t b2, uint32_t b1) {
    uint32_t cur = b1;
    while (cur != CFG_INVALID) {
        if (cur == b2) return 1;
        uint32_t nx = cfg->blocks[cur].idom;
        if (nx == cur) break;
        cur = nx;
    }
    return 0;
}

/* Allocate a bitset of `bc` blocks x `bc` rows. */
static uint64_t *alloc_df(size_t bc, size_t *words_out) {
    size_t words = (bc + 63) / 64;
    *words_out = words;
    if (!words) words = 1;
    return (uint64_t *)calloc(bc * words, sizeof(uint64_t));
}

static void df_set(uint64_t *df, size_t words, uint32_t row, uint32_t col) {
    df[(size_t)row * words + (col >> 6)] |= BIT(col & 63);
}

static int df_test(const uint64_t *df, size_t words,
                   uint32_t row, uint32_t col) {
    return (df[(size_t)row * words + (col >> 6)] >> (col & 63)) & 1u;
}

static int compute_df(const Cfg *cfg, uint64_t **df_out, size_t *words_out) {
    size_t bc = cfg->block_count;
    uint64_t *df = alloc_df(bc, words_out);
    if (!df) return -1;

    /* For each block n with >= 2 predecessors, for each predecessor p,
     * walk p up the idom chain until we reach idom[n]; add n to DF of
     * each visited node. */
    for (size_t n = 0; n < bc; n++) {
        /* Count preds; accumulate list. */
        size_t np = 0;
        for (size_t e = 0; e < cfg->edge_count; e++) {
            if (cfg->edges[e].to == (uint32_t)n &&
                cfg->edges[e].from != CFG_INVALID) np++;
        }
        if (np < 2) continue;
        uint32_t idom_n = cfg->blocks[n].idom;
        for (size_t e = 0; e < cfg->edge_count; e++) {
            const CfgEdge *ed = &cfg->edges[e];
            if (ed->to != (uint32_t)n) continue;
            if (ed->from == CFG_INVALID) continue;
            uint32_t runner = ed->from;
            while (runner != idom_n && runner != CFG_INVALID) {
                df_set(df, *words_out, runner, (uint32_t)n);
                uint32_t nx = cfg->blocks[runner].idom;
                if (nx == runner) break;
                runner = nx;
            }
        }
    }
    (void)dominates;
    *df_out = df;
    return 0;
}

/* Compute iterated DF for the set of blocks that define `reg`, mark
 * those (that are also live-in) as needing a phi. `is_gpr` selects
 * which register file. */
static int place_for_register(Ssa *ssa, const uint64_t *df, size_t words,
                              unsigned reg, int is_gpr) {
    size_t bc = ssa->cfg->block_count;
    uint8_t *has_phi = (uint8_t *)calloc(bc ? bc : 1, 1);
    uint8_t *worklist = (uint8_t *)calloc(bc ? bc : 1, 1);
    if (!has_phi || !worklist) { free(has_phi); free(worklist); return -1; }

    /* Seed: every block that defines this register. */
    for (size_t b = 0; b < bc; b++) {
        uint64_t defs = is_gpr ? ssa->blocks[b].gpr_defs
                               : ssa->blocks[b].fpr_defs;
        if (defs & BIT(reg)) worklist[b] = 1;
    }

    int changed = 1;
    while (changed) {
        changed = 0;
        for (size_t b = 0; b < bc; b++) {
            if (!worklist[b]) continue;
            worklist[b] = 0;
            for (size_t n = 0; n < bc; n++) {
                if (!df_test(df, words, (uint32_t)b, (uint32_t)n)) continue;
                if (has_phi[n]) continue;
                uint64_t live = is_gpr ? ssa->blocks[n].gpr_live_in
                                       : ssa->blocks[n].fpr_live_in;
                if (!(live & BIT(reg))) continue;
                has_phi[n] = 1;
                changed = 1;
                worklist[n] = 1;
                /* Record phi on the block summary and grow phi list. */
                if (is_gpr) ssa->blocks[n].gpr_phi |= BIT(reg);
                else        ssa->blocks[n].fpr_phi |= BIT(reg);
                size_t ns = ssa->phi_count + 1;
                SsaPhi *np = (SsaPhi *)realloc(ssa->phis,
                                               ns * sizeof(SsaPhi));
                if (!np) { free(has_phi); free(worklist); return -1; }
                ssa->phis = np;
                ssa->phis[ssa->phi_count].reg_kind = (uint8_t)(is_gpr ? 0 : 1);
                ssa->phis[ssa->phi_count].reg      = (uint8_t)reg;
                ssa->phis[ssa->phi_count].block    = (uint32_t)n;
                ssa->phi_count++;
            }
        }
    }
    free(has_phi); free(worklist);
    return 0;
}

int ssa_place_phis(Ssa *ssa) {
    size_t bc = ssa->cfg->block_count;
    if (bc == 0) return 0;

    uint64_t *df = NULL;
    size_t words = 0;
    if (compute_df(ssa->cfg, &df, &words) != 0) return -1;

    for (unsigned r = 0; r < SSA_MAX_GPR; r++) {
        if (place_for_register(ssa, df, words, r, 1) != 0) {
            free(df); return -1;
        }
    }
    for (unsigned r = 0; r < SSA_MAX_FPR; r++) {
        if (place_for_register(ssa, df, words, r, 0) != 0) {
            free(df); return -1;
        }
    }

    free(df);
    return 0;
}
