#include "cfg_internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Pattern we recognize for CodeWarrior-style jump tables:
 *
 *    cmplwi / cmpwi rN, <limit>
 *    bc (bgt / bge) -> <default label>
 *    ...
 *    addis  rT, 0,  HI(table)
 *    addi   rT, rT, LO(table)   ; or ori
 *    <load from rT at offset 4*rN>
 *    mtctr
 *    bctr
 *
 * Our decoder currently recognizes the terminating `bctr`, the
 * `addi`/`addis` immediate constants, the conditional branch, and the
 * `cmpwi` for the bounds check. The table itself is a contiguous run
 * of 4-byte big-endian absolute code addresses in a .data section.
 *
 * We take a pragmatic approach: for each block ending in bctr, walk
 * backward up to 16 instructions looking for addis+addi with matching
 * target register. If found, use that as the table base. Recover cases
 * by reading from the table until a word stops being a valid code
 * address inside the function.
 */

static int vaddr_in_text(const DolImage *img, uint32_t va) {
    const DolSection *s = dol_section_for_vaddr(img, va);
    if (!s) return 0;
    return s->kind == DOL_SECTION_TEXT;
}

static int find_table_base(const DolImage *img, const CfgBlock *b,
                           uint32_t *out_table) {
    /* Walk backward through the block's instructions. */
    uint32_t va = b->end_vaddr - 4u;       /* last insn = bctr */
    uint32_t stop = b->start_vaddr;
    uint32_t hi = 0; int have_hi = 0; uint8_t hi_reg = 0;

    /* Scan up to 16 instructions back. */
    unsigned walked = 0;
    while (walked++ < 16 && va > stop) {
        va -= 4u;
        int ok = 0;
        uint32_t w = cfg_read_word(img, va, &ok);
        if (!ok) return 0;
        PpcInsn in;
        ppc_decode(w, va, &in);
        if (in.op == PPC_OP_ADDIS && in.ra == 0) {
            /* lis rD, HI */
            hi = ((uint32_t)(int32_t)in.imm) << 16;
            hi_reg = in.rd;
            have_hi = 1;
            continue;
        }
        if (have_hi && in.op == PPC_OP_ADDI && in.ra == hi_reg &&
            in.rd == hi_reg) {
            *out_table = hi + (uint32_t)in.imm;
            return 1;
        }
        if (have_hi && in.op == PPC_OP_ORI && in.ra == hi_reg &&
            in.rd == hi_reg) {
            *out_table = hi | (uint32_t)in.imm;
            return 1;
        }
    }
    return 0;
}

/* Heuristic: a case entry is a 32-bit word that is 4-byte aligned and
 * falls inside a text section. We stop reading at the first non-matching
 * word, or after 256 entries (sanity limit). */
static int recover_cases(const DolImage *img, uint32_t table_va,
                         CfgSwitchCase **out, size_t *out_n) {
    size_t cap = 8, n = 0;
    CfgSwitchCase *arr = (CfgSwitchCase *)malloc(cap * sizeof(CfgSwitchCase));
    if (!arr) return -1;
    for (unsigned i = 0; i < 256; i++) {
        uint32_t va = table_va + i * 4u;
        int ok = 0;
        uint32_t w = cfg_read_word(img, va, &ok);
        if (!ok) break;
        if ((w & 3) != 0) break;
        if (!vaddr_in_text(img, w)) break;
        if (n == cap) {
            cap *= 2;
            CfgSwitchCase *r = (CfgSwitchCase *)realloc(arr,
                cap * sizeof(CfgSwitchCase));
            if (!r) { free(arr); return -1; }
            arr = r;
        }
        arr[n].case_value = i;
        arr[n].target_vaddr = w;
        n++;
    }
    if (n == 0) { free(arr); return 0; }
    *out = arr;
    *out_n = n;
    return 1;
}

static int register_switch(Cfg *cfg, uint32_t bi, uint32_t table_va,
                           CfgSwitchCase *cases, size_t ncases) {
    /* Replace the INDIRECT_UNKNOWN edge with one SWITCH_CASE per entry
     * that corresponds to a known block; if a target is not a block
     * start (internal label), we skip it (would require block splitting
     * mid-function, which is out of scope for this pass). */
    for (size_t i = 0; i < cfg->edge_count; ) {
        if (cfg->edges[i].from == bi &&
            cfg->edges[i].kind == CFG_EDGE_INDIRECT_UNKNOWN) {
            /* remove it */
            memmove(&cfg->edges[i], &cfg->edges[i + 1],
                    (cfg->edge_count - i - 1) * sizeof(CfgEdge));
            cfg->edge_count--;
        } else {
            i++;
        }
    }

    size_t recognized = 0;
    for (size_t i = 0; i < ncases; i++) {
        uint32_t t = cfg_block_of_vaddr(cfg, cases[i].target_vaddr);
        if (t == CFG_INVALID) continue;
        if (cfg_add_edge(cfg, bi, t, CFG_EDGE_SWITCH_CASE,
                         (int32_t)cases[i].case_value) != 0) return -1;
        recognized++;
    }

    size_t sn = cfg->switch_count + 1;
    CfgSwitch *ns = (CfgSwitch *)realloc(cfg->switches, sn * sizeof(CfgSwitch));
    if (!ns) return -1;
    cfg->switches = ns;
    cfg->switches[cfg->switch_count].block_index = bi;
    cfg->switches[cfg->switch_count].table_vaddr = table_va;
    cfg->switches[cfg->switch_count].default_vaddr = 0;
    cfg->switches[cfg->switch_count].cases = cases;
    cfg->switches[cfg->switch_count].case_count = ncases;
    cfg->switch_count++;

    cfg->blocks[bi].region_kind = CFG_REGION_SWITCH;
    (void)recognized;
    return 0;
}

int cfg_recover_switches(Cfg *cfg, const DolImage *img) {
    int recovered = 0;
    for (size_t bi = 0; bi < cfg->block_count; bi++) {
        const CfgBlock *b = &cfg->blocks[bi];
        if (b->insn_count == 0) continue;
        int ok = 0;
        uint32_t w = cfg_read_word(img, b->end_vaddr - 4u, &ok);
        if (!ok) continue;
        PpcInsn in;
        ppc_decode(w, b->end_vaddr - 4u, &in);
        if (in.op != PPC_OP_BCTR) continue;

        uint32_t table_va = 0;
        if (!find_table_base(img, b, &table_va)) continue;

        CfgSwitchCase *cases = NULL;
        size_t ncases = 0;
        int r = recover_cases(img, table_va, &cases, &ncases);
        if (r <= 0) continue;

        if (register_switch(cfg, (uint32_t)bi, table_va,
                            cases, ncases) != 0) {
            free(cases);
            return -1;
        }
        recovered++;
    }
    return recovered;
}
