/*
 * Function body emitter (req 8.4) + control-flow mapping (req 8.5).
 *
 * We render one Calynda function per symbol, with the signature
 * derived from the extern table entry when available (falling back
 * to `(args) -> ()`).  The body walks CFG blocks in program order,
 * emitting:
 *
 *   - linear statements derived from SSA expression trees for every
 *     side-effecting insn (stores, void calls, returns);
 *   - control-flow comments / block labels so the structure is
 *     self-documenting even when a region is irreducible;
 *   - if/else ternaries for reconverging conditionals;
 *   - `# loop` banners + tail-call hints for natural loops;
 *   - `discard _` for calls whose result is never consumed.
 *
 * This emitter intentionally keeps the body deterministic and
 * line-by-line readable rather than attempting a full re-structuring
 * pass; the goal is a scaffold a human (or a follow-up requirement)
 * can refine, not a final high-level translation.
 */
#include "emit_cal_internal.h"
#include "cli/demangle.h"
#include "cli/externs.h"
#include "cli/cfg.h"
#include "cli/ssa.h"
#include "ppc.h"

#include <ctype.h>
#include <string.h>

static const ExternSymbol *lookup_extern(const Externs *ex, const char *raw) {
    if (!ex || !raw) return NULL;
    for (size_t i = 0; i < ex->count; i++)
        if (strcmp(ex->entries[i].raw_name, raw) == 0)
            return &ex->entries[i];
    return NULL;
}

static const char *region_name(CfgRegionKind k) {
    switch (k) {
        case CFG_REGION_LINEAR:       return "linear";
        case CFG_REGION_IF:           return "if";
        case CFG_REGION_IF_ELSE:      return "if/else";
        case CFG_REGION_LOOP:         return "loop";
        case CFG_REGION_SWITCH:       return "switch";
        case CFG_REGION_UNSTRUCTURED: return "unstructured";
    }
    return "?";
}

static void signature_for(const ExternSymbol *ex, const char *raw,
                          char *out, size_t n) {
    if (ex && ex->cal_sig[0]) {
        snprintf(out, n, "%s", ex->cal_sig);
        return;
    }
    /* Fallback: treat first 4 GPR args as i32 placeholders. */
    (void)raw;
    snprintf(out, n, "fn(a: i32, b: i32, c: i32, d: i32) -> i32");
}

static const char *successor_label(const Cfg *cfg, uint32_t from,
                                   CfgEdgeKind kind, uint32_t *to_out) {
    for (size_t i = 0; i < cfg->edge_count; i++) {
        const CfgEdge *e = &cfg->edges[i];
        if (e->from != from) continue;
        if (e->kind == kind) { *to_out = e->to; return "match"; }
    }
    *to_out = CFG_INVALID;
    return NULL;
}

/* Emit statements for every side-effecting insn in the block.
 * We treat stores and calls as side-effecting. Pure computations
 * (their expr strings) are consumed by their downstream insn. */
static void emit_block_body(EmitFile *ef, const Ssa *ssa,
                            const SsaBlockInfo *b, const char *indent) {
    for (size_t k = 0; k < b->insn_count; k++) {
        const SsaInsn *si = &ssa->insns[b->first_insn + k];
        const PpcInsn *in = &si->in;
        const char *e = si->expr;

        switch (in->op) {
            case PPC_OP_STW: case PPC_OP_STB: case PPC_OP_STH:
            case PPC_OP_STWU: case PPC_OP_STBU: case PPC_OP_STHU:
            case PPC_OP_STFS: case PPC_OP_STFD:
            case PPC_OP_STFSU: case PPC_OP_STFDU:
                if (e) emit_linef(ef, "%s%s", indent, e);
                break;

            case PPC_OP_BL: case PPC_OP_BLA: {
                const SsaCallSite *cs = &ssa->calls[si->call_site_id];
                if (e) {
                    if (cs->returns_gpr || cs->returns_fpr)
                        emit_linef(ef, "%svar _r = %s", indent, e);
                    else
                        emit_linef(ef, "%sdiscard %s", indent, e);
                }
                break;
            }

            case PPC_OP_BLR:
                /* Handled by the block trailer as `return`. */
                break;

            case PPC_OP_MTLR: case PPC_OP_MTCTR:
                /* Prologue/epilogue glue — emit as a comment so the
                 * reader can trace back to the original insn. */
                emit_linef(ef, "%s# %s r%u  (ABI register save)",
                           indent, in->op == PPC_OP_MTLR ? "mtlr" : "mtctr",
                           in->rd);
                break;

            default:
                /* Pure computation: skip. Its expression will be
                 * inlined into the consumer that actually uses it. */
                break;
        }
    }
}

static void emit_block_trailer(EmitFile *ef, const Cfg *cfg, const Ssa *ssa,
                               const SsaBlockInfo *b, size_t bi,
                               const char *indent) {
    /* Find the last instruction to decide how control leaves. */
    if (b->insn_count == 0) return;
    const SsaInsn *last = &ssa->insns[b->first_insn + b->insn_count - 1];
    const char *g3 = "r3";  /* default return slot rendering */

    switch (last->in.op) {
        case PPC_OP_BLR:
            emit_linef(ef, "%sreturn %s", indent, g3);
            return;
        case PPC_OP_BCTR:
            emit_linef(ef, "%s# bctr: indirect tail call (unresolved)",
                       indent);
            emit_linef(ef, "%sreturn ()", indent);
            return;
        default: break;
    }

    uint32_t t_taken = CFG_INVALID, t_fall = CFG_INVALID, t_uncond = CFG_INVALID;
    successor_label(cfg, (uint32_t)bi, CFG_EDGE_COND_TAKEN, &t_taken);
    successor_label(cfg, (uint32_t)bi, CFG_EDGE_COND_NOT_TAKEN, &t_fall);
    successor_label(cfg, (uint32_t)bi, CFG_EDGE_UNCOND, &t_uncond);

    const CfgBlock *cb = &cfg->blocks[bi];
    if (cb->region_kind == CFG_REGION_IF ||
        cb->region_kind == CFG_REGION_IF_ELSE) {
        emit_linef(ef, "%s# %s region: merge at block %u",
                   indent, region_name(cb->region_kind), cb->if_merge);
        if (t_taken != CFG_INVALID)
            emit_linef(ef, "%s#   taken     -> block %u", indent, t_taken);
        if (t_fall != CFG_INVALID)
            emit_linef(ef, "%s#   not-taken -> block %u", indent, t_fall);
    } else if (cb->region_kind == CFG_REGION_LOOP) {
        emit_linef(ef, "%s# loop header (depth %u) — "
                       "tail-recursion candidate",
                   indent, cb->loop_depth);
    } else if (cb->region_kind == CFG_REGION_SWITCH) {
        emit_linef(ef, "%s# switch region (see jump table)", indent);
    } else if (t_uncond != CFG_INVALID) {
        emit_linef(ef, "%s# goto block %u", indent, t_uncond);
    }
}

int emit_function(EmitFile *ef, const DolImage *img,
                  const SymTable *syms, const Externs *ex,
                  const SymEntry *fn) {
    if (fn->size == 0) return 0;                 /* unknown length */

    const DolSection *sec = dol_section_for_vaddr(img, fn->vaddr);
    if (!sec || sec->kind != DOL_SECTION_TEXT) return 0;

    /* Build CFG + SSA for this function. Both are local to this call
     * so we free them on every return path. */
    Cfg cfg;
    if (cfg_build(img, fn->vaddr, fn->vaddr + fn->size, &cfg) != 0)
        return -1;
    Ssa ssa;
    if (ssa_build(img, &cfg, &ssa) != 0) { cfg_free(&cfg); return -1; }

    char ident[SYM_NAME_MAX];
    emit_safe_ident(fn->name, ident, sizeof ident);

    const ExternSymbol *ex_entry = lookup_extern(ex, fn->name);
    char sig[512];
    signature_for(ex_entry, fn->name, sig, sizeof sig);

    emit_linef(ef, "# --- function %s (req 8.4) ---", fn->name);
    emit_linef(ef, "# 0x%08x  %u bytes  %zu blocks  %zu edges",
               fn->vaddr, fn->size, cfg.block_count, cfg.edge_count);
    if (cfg.is_irreducible)
        emit_linef(ef, "# warning: irreducible CFG — "
                       "emitted as sequential blocks");

    /* Map the extern sig "fn(a:i32,...)->T" to "(a:i32,...) -> T { ... };"
     * style. If we only have the sig, we splice it into the binding. */
    emit_linef(ef, "export %s = %s -> {", ident, sig);

    for (size_t bi = 0; bi < cfg.block_count; bi++) {
        const SsaBlockInfo *b = &ssa.blocks[bi];
        const CfgBlock *cb = &cfg.blocks[bi];
        if (bi > 0)
            emit_linef(ef, "    # block %zu: 0x%08x..0x%08x  (%s)",
                       bi, cb->start_vaddr, cb->end_vaddr,
                       region_name(cb->region_kind));
        emit_block_body   (ef, &ssa, b, "    ");
        emit_block_trailer(ef, &cfg, &ssa, b, bi, "    ");
    }

    emit_linef(ef, "};");
    emit_linef(ef, "");

    ssa_free(&ssa);
    cfg_free(&cfg);
    (void)syms;
    return 1;
}
