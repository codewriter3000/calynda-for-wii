#include "cli/cfg.h"

#include <stdio.h>

static const char *edge_name(CfgEdgeKind k) {
    switch (k) {
        case CFG_EDGE_FALLTHROUGH:      return "fall";
        case CFG_EDGE_UNCOND:           return "uncond";
        case CFG_EDGE_COND_TAKEN:       return "cond-taken";
        case CFG_EDGE_COND_NOT_TAKEN:   return "cond-ft";
        case CFG_EDGE_SWITCH_CASE:      return "switch";
        case CFG_EDGE_SWITCH_DEFAULT:   return "switch-def";
        case CFG_EDGE_INDIRECT_UNKNOWN: return "indirect?";
        case CFG_EDGE_RETURN:           return "return";
    }
    return "?";
}

static const char *region_name(CfgRegionKind k) {
    switch (k) {
        case CFG_REGION_LINEAR:         return "linear";
        case CFG_REGION_IF:             return "if";
        case CFG_REGION_IF_ELSE:        return "if-else";
        case CFG_REGION_LOOP:           return "loop";
        case CFG_REGION_SWITCH:         return "switch";
        case CFG_REGION_UNSTRUCTURED:   return "unstructured";
    }
    return "?";
}

void cfg_print(const Cfg *cfg, FILE *out) {
    if (!cfg || !out) return;
    fprintf(out, "# CFG: %zu blocks, %zu edges",
            cfg->block_count, cfg->edge_count);
    if (cfg->is_irreducible) fprintf(out, "  [IRREDUCIBLE]");
    if (cfg->switch_count)
        fprintf(out, "  [switches=%zu]", cfg->switch_count);
    fprintf(out, "\n");

    for (size_t i = 0; i < cfg->block_count; i++) {
        const CfgBlock *b = &cfg->blocks[i];
        fprintf(out, "B%zu  0x%08x..0x%08x  insns=%u  idom=",
                i, b->start_vaddr, b->end_vaddr, b->insn_count);
        if (b->idom == CFG_INVALID) fprintf(out, "-");
        else if (b->idom == (uint32_t)i) fprintf(out, "self");
        else fprintf(out, "B%u", b->idom);
        fprintf(out, "  depth=%u  %s%s%s",
                (unsigned)b->loop_depth, region_name(b->region_kind),
                b->is_loop_header ? "  [header]" : "",
                b->is_loop_exit ? "  [exit]" : "");
        if (b->if_merge != CFG_INVALID)
            fprintf(out, "  merge=B%u", b->if_merge);
        fprintf(out, "\n");
    }

    fprintf(out, "edges:\n");
    for (size_t i = 0; i < cfg->edge_count; i++) {
        const CfgEdge *e = &cfg->edges[i];
        fprintf(out, "  B%u -> ", e->from);
        if (e->to == CFG_INVALID) fprintf(out, "exit");
        else                      fprintf(out, "B%u", e->to);
        fprintf(out, "  (%s", edge_name(e->kind));
        if (e->kind == CFG_EDGE_SWITCH_CASE)
            fprintf(out, " case=%d", e->case_value);
        fprintf(out, ")\n");
    }

    for (size_t i = 0; i < cfg->switch_count; i++) {
        const CfgSwitch *s = &cfg->switches[i];
        fprintf(out, "switch at B%u  table=0x%08x  cases=%zu:\n",
                s->block_index, s->table_vaddr, s->case_count);
        for (size_t j = 0; j < s->case_count; j++) {
            fprintf(out, "  [%u] -> 0x%08x\n",
                    s->cases[j].case_value, s->cases[j].target_vaddr);
        }
    }
}
