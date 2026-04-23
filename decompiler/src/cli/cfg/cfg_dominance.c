#include "cfg_internal.h"

#include <stdlib.h>
#include <string.h>

/* Build predecessor list (CSR-style) from edge array. */
typedef struct { uint32_t *offsets; uint32_t *preds; size_t total; } Preds;

static int build_preds(const Cfg *cfg, Preds *p) {
    size_t nb = cfg->block_count;
    p->offsets = (uint32_t *)calloc(nb + 1, sizeof(uint32_t));
    if (!p->offsets) return -1;
    for (size_t i = 0; i < cfg->edge_count; i++) {
        uint32_t to = cfg->edges[i].to;
        if (to == CFG_INVALID) continue;
        p->offsets[to + 1]++;
    }
    for (size_t i = 1; i <= nb; i++) p->offsets[i] += p->offsets[i - 1];
    p->total = p->offsets[nb];
    p->preds = (uint32_t *)malloc(p->total * sizeof(uint32_t));
    if (!p->preds) { free(p->offsets); return -1; }
    uint32_t *cursor = (uint32_t *)calloc(nb, sizeof(uint32_t));
    if (!cursor) { free(p->offsets); free(p->preds); return -1; }
    for (size_t i = 0; i < cfg->edge_count; i++) {
        const CfgEdge *e = &cfg->edges[i];
        if (e->to == CFG_INVALID) continue;
        p->preds[p->offsets[e->to] + cursor[e->to]++] = e->from;
    }
    free(cursor);
    return 0;
}

static void free_preds(Preds *p) { free(p->offsets); free(p->preds); }

/* Reverse post-order traversal of the CFG, rooted at entry_block. */
typedef struct { uint32_t *order; uint32_t *pos; size_t count; } RPO;

static void dfs_post(const Cfg *cfg, uint32_t bi, uint8_t *seen,
                     RPO *rpo) {
    if (seen[bi]) return;
    seen[bi] = 1;
    for (size_t i = 0; i < cfg->edge_count; i++) {
        if (cfg->edges[i].from != bi) continue;
        uint32_t s = cfg->edges[i].to;
        if (s == CFG_INVALID) continue;
        dfs_post(cfg, s, seen, rpo);
    }
    rpo->order[rpo->count++] = bi;
}

static int build_rpo(const Cfg *cfg, RPO *rpo) {
    size_t nb = cfg->block_count;
    rpo->order = (uint32_t *)malloc(nb * sizeof(uint32_t));
    rpo->pos   = (uint32_t *)malloc(nb * sizeof(uint32_t));
    if (!rpo->order || !rpo->pos) { free(rpo->order); free(rpo->pos); return -1; }
    rpo->count = 0;
    uint8_t *seen = (uint8_t *)calloc(nb, 1);
    if (!seen) { free(rpo->order); free(rpo->pos); return -1; }
    dfs_post(cfg, cfg->entry_block, seen, rpo);
    free(seen);
    /* Reverse to get reverse-post-order; position maps block -> rpo index. */
    for (size_t i = 0; i < rpo->count / 2; i++) {
        uint32_t t = rpo->order[i];
        rpo->order[i] = rpo->order[rpo->count - 1 - i];
        rpo->order[rpo->count - 1 - i] = t;
    }
    for (size_t i = 0; i < nb; i++) rpo->pos[i] = CFG_INVALID;
    for (size_t i = 0; i < rpo->count; i++) rpo->pos[rpo->order[i]] = (uint32_t)i;
    return 0;
}

static uint32_t intersect(uint32_t b1, uint32_t b2,
                          const uint32_t *idom, const uint32_t *rpo_pos) {
    while (b1 != b2) {
        while (rpo_pos[b1] > rpo_pos[b2]) b1 = idom[b1];
        while (rpo_pos[b2] > rpo_pos[b1]) b2 = idom[b2];
    }
    return b1;
}

int cfg_compute_dominance(Cfg *cfg) {
    size_t nb = cfg->block_count;
    if (nb == 0) return 0;

    Preds P; if (build_preds(cfg, &P) != 0) return -1;
    RPO   R; if (build_rpo(cfg, &R)    != 0) { free_preds(&P); return -1; }

    uint32_t *idom = (uint32_t *)malloc(nb * sizeof(uint32_t));
    if (!idom) { free_preds(&P); free(R.order); free(R.pos); return -1; }
    for (size_t i = 0; i < nb; i++) idom[i] = CFG_INVALID;
    idom[cfg->entry_block] = cfg->entry_block;

    int changed = 1;
    while (changed) {
        changed = 0;
        for (size_t ri = 0; ri < R.count; ri++) {
            uint32_t b = R.order[ri];
            if (b == cfg->entry_block) continue;
            uint32_t new_idom = CFG_INVALID;
            uint32_t beg = P.offsets[b], end = P.offsets[b + 1];
            for (uint32_t k = beg; k < end; k++) {
                uint32_t p = P.preds[k];
                if (idom[p] == CFG_INVALID) continue;
                new_idom = (new_idom == CFG_INVALID)
                           ? p : intersect(p, new_idom, idom, R.pos);
            }
            if (new_idom != CFG_INVALID && idom[b] != new_idom) {
                idom[b] = new_idom; changed = 1;
            }
        }
    }
    for (size_t i = 0; i < nb; i++) cfg->blocks[i].idom = idom[i];

    free(idom); free_preds(&P); free(R.order); free(R.pos);
    return 0;
}

/* A dominates B iff walking idom chain from B reaches A. */
static int dominates(const Cfg *cfg, uint32_t a, uint32_t b) {
    while (b != CFG_INVALID) {
        if (b == a) return 1;
        uint32_t nx = cfg->blocks[b].idom;
        if (nx == b) return a == b;
        b = nx;
    }
    return 0;
}

int cfg_classify_loops(Cfg *cfg) {
    size_t nb = cfg->block_count;
    if (nb == 0) return 0;
    uint16_t *depth = (uint16_t *)calloc(nb, sizeof(uint16_t));
    if (!depth) return -1;

    /* Back-edge: edge from->to where `to` dominates `from`. */
    for (size_t i = 0; i < cfg->edge_count; i++) {
        const CfgEdge *e = &cfg->edges[i];
        if (e->to == CFG_INVALID) continue;
        if (!dominates(cfg, e->to, e->from)) continue;
        cfg->blocks[e->to].is_loop_header = 1;
        /* Any back-edge whose head does NOT dominate tail would make
         * the CFG irreducible; here we know head DOES dominate tail,
         * so this back-edge is reducible. We'll check the converse
         * below: a reachable SCC in the reverse CFG that contains a
         * non-back-edge entry into a loop body would flag it. For now,
         * rely on an additional check: every edge whose target is a
         * loop body but not a header, and whose source is outside the
         * loop, indicates multi-entry (irreducible). This is checked
         * in cfg_classify_regions. */
    }

    /* Depth = number of loop headers that dominate this block. */
    for (size_t i = 0; i < nb; i++) {
        uint16_t d = 0;
        uint32_t cur = (uint32_t)i;
        while (cur != CFG_INVALID) {
            if (cfg->blocks[cur].is_loop_header && cur != (uint32_t)i) d++;
            uint32_t nx = cfg->blocks[cur].idom;
            if (nx == cur) break;
            cur = nx;
        }
        depth[i] = d;
    }
    for (size_t i = 0; i < nb; i++) cfg->blocks[i].loop_depth = depth[i];

    /* Loop exit: has a successor at strictly lower loop depth. */
    for (size_t i = 0; i < cfg->edge_count; i++) {
        const CfgEdge *e = &cfg->edges[i];
        if (e->to == CFG_INVALID) continue;
        if (cfg->blocks[e->to].loop_depth < cfg->blocks[e->from].loop_depth)
            cfg->blocks[e->from].is_loop_exit = 1;
    }

    free(depth);
    return 0;
}
