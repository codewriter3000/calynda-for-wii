#include "cfg_internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

uint32_t cfg_read_word(const DolImage *img, uint32_t vaddr, int *ok) {
    size_t rem = 0;
    const uint8_t *p = dol_vaddr_to_ptr(img, vaddr, &rem);
    if (!p || rem < 4) { *ok = 0; return 0; }
    *ok = 1;
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] <<  8) |  (uint32_t)p[3];
}

uint32_t cfg_block_of_vaddr(const Cfg *cfg, uint32_t vaddr) {
    for (size_t i = 0; i < cfg->block_count; i++) {
        if (cfg->blocks[i].start_vaddr == vaddr) return (uint32_t)i;
    }
    return CFG_INVALID;
}

uint32_t cfg_add_block(Cfg *cfg, uint32_t start_vaddr) {
    size_t n = cfg->block_count + 1;
    CfgBlock *nb = (CfgBlock *)realloc(cfg->blocks, n * sizeof(CfgBlock));
    if (!nb) return CFG_INVALID;
    cfg->blocks = nb;
    memset(&cfg->blocks[cfg->block_count], 0, sizeof(CfgBlock));
    cfg->blocks[cfg->block_count].start_vaddr = start_vaddr;
    cfg->blocks[cfg->block_count].idom = CFG_INVALID;
    cfg->blocks[cfg->block_count].if_merge = CFG_INVALID;
    return (uint32_t)cfg->block_count++;
}

int cfg_add_edge(Cfg *cfg, uint32_t from, uint32_t to,
                 CfgEdgeKind kind, int32_t case_value) {
    size_t n = cfg->edge_count + 1;
    CfgEdge *ne = (CfgEdge *)realloc(cfg->edges, n * sizeof(CfgEdge));
    if (!ne) return -1;
    cfg->edges = ne;
    cfg->edges[cfg->edge_count].from = from;
    cfg->edges[cfg->edge_count].to   = to;
    cfg->edges[cfg->edge_count].kind = kind;
    cfg->edges[cfg->edge_count].case_value = case_value;
    cfg->edge_count++;
    return 0;
}

/* Leader collection: scan every instruction once to find the set of
 * addresses that must begin a block. */
static int collect_leaders(const DolImage *img,
                           uint32_t start, uint32_t end,
                           uint32_t **out, size_t *out_n) {
    size_t cap = 16, n = 0;
    uint32_t *arr = (uint32_t *)malloc(cap * sizeof(uint32_t));
    if (!arr) return -1;

    #define PUSH(v) do {                                     \
        if (n == cap) {                                      \
            cap *= 2;                                        \
            uint32_t *r = (uint32_t *)realloc(arr,           \
                              cap * sizeof(uint32_t));       \
            if (!r) { free(arr); return -1; }                \
            arr = r;                                         \
        }                                                    \
        arr[n++] = (v);                                      \
    } while (0)

    PUSH(start);

    for (uint32_t va = start; va < end; va += 4) {
        int ok = 0;
        uint32_t w = cfg_read_word(img, va, &ok);
        if (!ok) return -1;
        PpcInsn in;
        ppc_decode(w, va, &in);
        switch (in.op) {
            case PPC_OP_B: case PPC_OP_BA: case PPC_OP_BC:
                if (in.target >= start && in.target < end) PUSH(in.target);
                if (va + 4 < end) PUSH(va + 4);
                break;
            case PPC_OP_BL: case PPC_OP_BLA:
                /* Calls do not start a new block in the caller. */
                break;
            case PPC_OP_BLR: case PPC_OP_BCTR:
                if (va + 4 < end) PUSH(va + 4);
                break;
            default: break;
        }
    }
    #undef PUSH

    /* Sort + dedupe. */
    for (size_t i = 1; i < n; i++) {
        uint32_t key = arr[i];
        size_t j = i;
        while (j > 0 && arr[j-1] > key) { arr[j] = arr[j-1]; j--; }
        arr[j] = key;
    }
    size_t w = 0;
    for (size_t i = 0; i < n; i++) {
        if (w == 0 || arr[w-1] != arr[i]) arr[w++] = arr[i];
    }

    *out = arr;
    *out_n = w;
    return 0;
}

/* Build blocks from leader list, then emit successor edges by decoding
 * the last instruction of each block. */
static int split_and_wire(Cfg *cfg, const DolImage *img,
                          uint32_t end,
                          const uint32_t *leaders, size_t nl) {
    for (size_t i = 0; i < nl; i++) {
        uint32_t start_va = leaders[i];
        uint32_t end_va   = (i + 1 < nl) ? leaders[i+1] : end;
        uint32_t idx = cfg_add_block(cfg, start_va);
        if (idx == CFG_INVALID) return -1;
        cfg->blocks[idx].end_vaddr = end_va;
        cfg->blocks[idx].insn_count = (end_va - start_va) / 4u;
    }
    cfg->entry_block = 0;

    for (size_t i = 0; i < cfg->block_count; i++) {
        CfgBlock *b = &cfg->blocks[i];
        uint32_t last_va = b->end_vaddr - 4u;
        int ok = 0;
        uint32_t w = cfg_read_word(img, last_va, &ok);
        if (!ok) return -1;
        PpcInsn in;
        ppc_decode(w, last_va, &in);

        uint32_t ft = b->end_vaddr;
        uint32_t ft_idx = cfg_block_of_vaddr(cfg, ft);

        switch (in.op) {
            case PPC_OP_B: case PPC_OP_BA: {
                uint32_t t = cfg_block_of_vaddr(cfg, in.target);
                if (t == CFG_INVALID)
                    cfg_add_edge(cfg, (uint32_t)i, CFG_INVALID,
                                 CFG_EDGE_RETURN, -1);  /* tail call */
                else
                    cfg_add_edge(cfg, (uint32_t)i, t,
                                 CFG_EDGE_UNCOND, -1);
                break;
            }
            case PPC_OP_BC: {
                uint32_t t = cfg_block_of_vaddr(cfg, in.target);
                if (t != CFG_INVALID)
                    cfg_add_edge(cfg, (uint32_t)i, t,
                                 CFG_EDGE_COND_TAKEN, -1);
                if (ft_idx != CFG_INVALID)
                    cfg_add_edge(cfg, (uint32_t)i, ft_idx,
                                 CFG_EDGE_COND_NOT_TAKEN, -1);
                break;
            }
            case PPC_OP_BLR:
                cfg_add_edge(cfg, (uint32_t)i, CFG_INVALID,
                             CFG_EDGE_RETURN, -1);
                break;
            case PPC_OP_BCTR:
                cfg_add_edge(cfg, (uint32_t)i, CFG_INVALID,
                             CFG_EDGE_INDIRECT_UNKNOWN, -1);
                break;
            default:
                if (ft_idx != CFG_INVALID)
                    cfg_add_edge(cfg, (uint32_t)i, ft_idx,
                                 CFG_EDGE_FALLTHROUGH, -1);
                else
                    cfg_add_edge(cfg, (uint32_t)i, CFG_INVALID,
                                 CFG_EDGE_RETURN, -1);  /* falls off */
                break;
        }
    }
    return 0;
}

int cfg_build(const DolImage *img,
              uint32_t start_vaddr, uint32_t end_vaddr,
              Cfg *out) {
    if (!img || !out || start_vaddr >= end_vaddr ||
        (start_vaddr & 3) || (end_vaddr & 3)) {
        fprintf(stderr, "cfg_build: bad args\n");
        return -1;
    }
    memset(out, 0, sizeof(*out));
    out->entry_block = CFG_INVALID;

    uint32_t *leaders = NULL;
    size_t nl = 0;
    if (collect_leaders(img, start_vaddr, end_vaddr, &leaders, &nl) != 0) {
        fprintf(stderr, "cfg_build: leader collection failed\n");
        return -1;
    }
    int rc = split_and_wire(out, img, end_vaddr, leaders, nl);
    free(leaders);
    if (rc != 0) { cfg_free(out); return -1; }

    if (cfg_compute_dominance(out) != 0) { cfg_free(out); return -1; }
    if (cfg_classify_loops(out)    != 0) { cfg_free(out); return -1; }
    if (cfg_recover_switches(out, img) < 0) { cfg_free(out); return -1; }
    if (cfg_classify_regions(out)  != 0) { cfg_free(out); return -1; }
    return 0;
}

void cfg_free(Cfg *cfg) {
    if (!cfg) return;
    free(cfg->blocks);
    free(cfg->edges);
    for (size_t i = 0; i < cfg->switch_count; i++)
        free(cfg->switches[i].cases);
    free(cfg->switches);
    memset(cfg, 0, sizeof(*cfg));
}
