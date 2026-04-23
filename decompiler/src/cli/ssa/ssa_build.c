/*
 * SSA orchestration: decode every instruction in every CFG block,
 * populate the flat SsaInsn array, then drive passes 2.1 -> 2.6.
 */
#include "ssa_internal.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* -------- string pool: owns each string independently so that
 * earlier pointers remain stable across any number of interns. -------- */

static int pool_track(Ssa *ssa, char *p) {
    if (ssa->expr_pool_len == ssa->expr_pool_cap) {
        size_t cap = ssa->expr_pool_cap ? ssa->expr_pool_cap * 2 : 64;
        char **np = (char **)realloc((char **)ssa->expr_pool,
                                     cap * sizeof(char *));
        if (!np) { free(p); return -1; }
        ssa->expr_pool = (char *)np;
        ssa->expr_pool_cap = cap;
    }
    ((char **)ssa->expr_pool)[ssa->expr_pool_len++] = p;
    return 0;
}

const char *ssa_intern(Ssa *ssa, const char *s) {
    size_t n = strlen(s) + 1;
    char *p = (char *)malloc(n);
    if (!p) return NULL;
    memcpy(p, s, n);
    if (pool_track(ssa, p) != 0) return NULL;
    return p;
}

const char *ssa_internf(Ssa *ssa, const char *fmt, ...) {
    char tmp[256];
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(tmp, sizeof(tmp), fmt, ap);
    va_end(ap);
    if (n < 0) return NULL;
    if ((size_t)n >= sizeof(tmp)) n = (int)sizeof(tmp) - 1;
    tmp[n] = 0;
    return ssa_intern(ssa, tmp);
}

/* -------- small shared helpers -------- */

unsigned ssa_popcount64(uint64_t v) {
    unsigned c = 0;
    while (v) { v &= v - 1; c++; }
    return c;
}

int ssa_block_of_pc(const Ssa *ssa, uint32_t pc, uint32_t *out) {
    for (size_t i = 0; i < ssa->cfg->block_count; i++) {
        const CfgBlock *b = &ssa->cfg->blocks[i];
        if (pc >= b->start_vaddr && pc < b->end_vaddr) {
            *out = (uint32_t)i;
            return 0;
        }
    }
    return -1;
}

/* -------- decode pass -------- */

static uint32_t read_word(const DolImage *img, uint32_t va, int *ok) {
    size_t rem = 0;
    const uint8_t *p = dol_vaddr_to_ptr(img, va, &rem);
    if (!p || rem < 4) { *ok = 0; return 0; }
    *ok = 1;
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] <<  8) |  (uint32_t)p[3];
}

static int decode_all_insns(Ssa *ssa) {
    size_t total = 0;
    for (size_t i = 0; i < ssa->cfg->block_count; i++)
        total += ssa->cfg->blocks[i].insn_count;

    ssa->insns = (SsaInsn *)calloc(total ? total : 1, sizeof(SsaInsn));
    ssa->blocks = (SsaBlockInfo *)calloc(ssa->cfg->block_count ? ssa->cfg->block_count : 1,
                                          sizeof(SsaBlockInfo));
    if (!ssa->insns || !ssa->blocks) return -1;
    ssa->insn_count = total;

    size_t cursor = 0;
    for (size_t i = 0; i < ssa->cfg->block_count; i++) {
        const CfgBlock *b = &ssa->cfg->blocks[i];
        SsaBlockInfo *bi = &ssa->blocks[i];
        bi->first_insn = cursor;
        bi->insn_count = b->insn_count;
        for (uint8_t r = 0; r < SSA_MAX_GPR; r++) bi->gpr_last_def[r] = SSA_NO_VAL;
        for (uint8_t r = 0; r < SSA_MAX_FPR; r++) bi->fpr_last_def[r] = SSA_NO_VAL;

        for (unsigned k = 0; k < b->insn_count; k++) {
            uint32_t pc = b->start_vaddr + k * 4u;
            int ok = 0;
            uint32_t w = read_word(ssa->img, pc, &ok);
            if (!ok) return -1;
            SsaInsn *si = &ssa->insns[cursor + k];
            si->pc = pc;
            ppc_decode(w, pc, &si->in);
            si->gpr_def = SSA_NO_REG;
            si->fpr_def = SSA_NO_REG;
            si->slot_id = SSA_NO_VAL;
            si->call_site_id = SSA_NO_VAL;
        }
        cursor += b->insn_count;
    }
    return 0;
}

/* -------- public entry points -------- */

int ssa_build(const DolImage *img, const Cfg *cfg, Ssa *out) {
    if (!img || !cfg || !out) return -1;
    memset(out, 0, sizeof(*out));
    out->img = img;
    out->cfg = cfg;

    if (decode_all_insns(out)       != 0) { ssa_free(out); return -1; }
    if (ssa_compute_defuse(out)     != 0) { ssa_free(out); return -1; }
    if (ssa_place_phis(out)         != 0) { ssa_free(out); return -1; }
    if (ssa_fold_prologue(out)      != 0) { ssa_free(out); return -1; }
    if (ssa_map_stack_slots(out)    != 0) { ssa_free(out); return -1; }
    if (ssa_reconstruct_calls(out)  != 0) { ssa_free(out); return -1; }
    if (ssa_build_expressions(out)  != 0) { ssa_free(out); return -1; }
    return 0;
}

void ssa_free(Ssa *ssa) {
    if (!ssa) return;
    free(ssa->insns);
    free(ssa->blocks);
    free(ssa->phis);
    free(ssa->slots);
    free(ssa->calls);
    if (ssa->expr_pool) {
        char **arr = (char **)ssa->expr_pool;
        for (size_t i = 0; i < ssa->expr_pool_len; i++) free(arr[i]);
        free(arr);
    }
    memset(ssa, 0, sizeof(*ssa));
}
