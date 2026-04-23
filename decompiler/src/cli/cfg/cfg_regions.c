#include "cfg_internal.h"

#include <stdlib.h>
#include <string.h>

/* True if edge from -> to is a back-edge (to dominates from). */
static int is_back(const Cfg *cfg, uint32_t from, uint32_t to) {
    uint32_t cur = from;
    while (cur != CFG_INVALID) {
        if (cur == to) return 1;
        uint32_t nx = cfg->blocks[cur].idom;
        if (nx == cur) break;
        cur = nx;
    }
    return 0;
}

/* Mark every block forward-reachable from `start` (skipping back-edges
 * and SWITCH_CASE successors, which we treat as a separate region). */
static void forward_reach(const Cfg *cfg, uint32_t start, uint8_t *mark) {
    size_t nb = cfg->block_count;
    uint32_t *stack = (uint32_t *)malloc(nb * sizeof(uint32_t));
    if (!stack) return;
    size_t top = 0;
    stack[top++] = start;
    while (top) {
        uint32_t b = stack[--top];
        if (mark[b]) continue;
        mark[b] = 1;
        for (size_t i = 0; i < cfg->edge_count; i++) {
            const CfgEdge *e = &cfg->edges[i];
            if (e->from != b || e->to == CFG_INVALID) continue;
            if (e->kind == CFG_EDGE_SWITCH_CASE) continue;
            if (is_back(cfg, e->from, e->to)) continue;
            if (!mark[e->to]) stack[top++] = e->to;
        }
    }
    free(stack);
}

/* Nearest forward-reachable block common to both s1 and s2 (BFS from
 * s2 returns the first hit against reach(s1)). */
static uint32_t first_common(const Cfg *cfg, uint32_t s1, uint32_t s2) {
    size_t nb = cfg->block_count;
    uint8_t *m1 = (uint8_t *)calloc(nb, 1);
    if (!m1) return CFG_INVALID;
    forward_reach(cfg, s1, m1);

    /* BFS from s2, return first node in m1. */
    uint32_t *q = (uint32_t *)malloc(nb * sizeof(uint32_t));
    uint8_t *seen = (uint8_t *)calloc(nb, 1);
    if (!q || !seen) { free(q); free(seen); free(m1); return CFG_INVALID; }
    size_t qh = 0, qt = 0;
    q[qt++] = s2; seen[s2] = 1;
    uint32_t result = CFG_INVALID;
    while (qh < qt) {
        uint32_t b = q[qh++];
        if (m1[b]) { result = b; break; }
        for (size_t i = 0; i < cfg->edge_count; i++) {
            const CfgEdge *e = &cfg->edges[i];
            if (e->from != b || e->to == CFG_INVALID) continue;
            if (e->kind == CFG_EDGE_SWITCH_CASE) continue;
            if (is_back(cfg, e->from, e->to)) continue;
            if (!seen[e->to]) { seen[e->to] = 1; q[qt++] = e->to; }
        }
    }
    free(q); free(seen); free(m1);
    return result;
}

/* Non-back-edge forward successors of `bi`. */
static void forward_succs(const Cfg *cfg, uint32_t bi,
                          uint32_t *s1, uint32_t *s2) {
    *s1 = *s2 = CFG_INVALID;
    for (size_t i = 0; i < cfg->edge_count; i++) {
        const CfgEdge *e = &cfg->edges[i];
        if (e->from != bi) continue;
        if (e->to == CFG_INVALID) continue;
        if (e->kind == CFG_EDGE_SWITCH_CASE ||
            e->kind == CFG_EDGE_SWITCH_DEFAULT) continue;
        if (is_back(cfg, e->from, e->to)) continue;
        if      (*s1 == CFG_INVALID) *s1 = e->to;
        else if (*s2 == CFG_INVALID) *s2 = e->to;
    }
}

/* A loop body block is multi-entry iff an edge enters the loop body
 * from outside and does not land on the header. */
static int detect_irreducible(const Cfg *cfg) {
    size_t nb = cfg->block_count;
    for (size_t h = 0; h < nb; h++) {
        if (!cfg->blocks[h].is_loop_header) continue;
        for (size_t i = 0; i < cfg->edge_count; i++) {
            const CfgEdge *e = &cfg->edges[i];
            if (e->to == CFG_INVALID) continue;
            if (e->to == h) continue;
            int to_in = 0, fr_in = 0;
            uint32_t cur;
            for (cur = e->to; cur != CFG_INVALID;) {
                if (cur == (uint32_t)h) { to_in = 1; break; }
                uint32_t nx = cfg->blocks[cur].idom;
                if (nx == cur) break;
                cur = nx;
            }
            for (cur = e->from; cur != CFG_INVALID;) {
                if (cur == (uint32_t)h) { fr_in = 1; break; }
                uint32_t nx = cfg->blocks[cur].idom;
                if (nx == cur) break;
                cur = nx;
            }
            if (to_in && !fr_in) return 1;
        }
    }
    return 0;
}

int cfg_classify_regions(Cfg *cfg) {
    size_t nb = cfg->block_count;
    if (nb == 0) return 0;

    if (detect_irreducible(cfg)) {
        cfg->is_irreducible = 1;
        for (size_t i = 0; i < nb; i++)
            cfg->blocks[i].region_kind = CFG_REGION_UNSTRUCTURED;
        return 0;
    }

    for (size_t i = 0; i < nb; i++) {
        CfgBlock *b = &cfg->blocks[i];
        if (b->region_kind == CFG_REGION_SWITCH) continue;

        if (b->is_loop_header) { b->region_kind = CFG_REGION_LOOP; continue; }

        uint32_t s1, s2;
        forward_succs(cfg, (uint32_t)i, &s1, &s2);
        if (s1 == CFG_INVALID || s2 == CFG_INVALID) {
            b->region_kind = CFG_REGION_LINEAR;
            continue;
        }

        uint32_t merge = first_common(cfg, s1, s2);
        if (merge == CFG_INVALID) {
            b->region_kind = CFG_REGION_UNSTRUCTURED;
            continue;
        }
        b->if_merge = merge;
        b->region_kind = (s1 == merge || s2 == merge)
                         ? CFG_REGION_IF : CFG_REGION_IF_ELSE;
    }
    return 0;
}
