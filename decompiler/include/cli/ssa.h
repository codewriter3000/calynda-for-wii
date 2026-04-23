/*
 * Static Single Assignment (SSA) analysis for a single decoded
 * PowerPC function.
 *
 * Given a CFG built by cfg_build() plus the source DolImage, ssa_build()
 * produces:
 *   - per-block def/use/live-in sets for the 32 GPRs and 32 FPRs
 *     (requirement 2.1);
 *   - phi-function placement at dominance-frontier joins (2.2);
 *   - folded 32-bit constants recovered from the standard
 *     addis+addi/ori prologue (2.3);
 *   - a lightweight expression-tree representation that inlines
 *     single-use register defs into their consumer (2.4);
 *   - a map of stack slots reached via `N(r1)` loads/stores, each
 *     assigned a stable local name (2.5);
 *   - call-site argument / return-value reconstruction for bl
 *     instructions, using the SysV PPC convention (2.6).
 *
 * The representation is deliberately compact: one SsaInsn per decoded
 * instruction, and side-tables per block and per call site. The goal
 * is to be a foundation on which later structuring / re-emission
 * passes can build, not to be a full compiler IR.
 */
#ifndef CALYNDA_DECOMPILER_CLI_SSA_H
#define CALYNDA_DECOMPILER_CLI_SSA_H

#include "cli/cfg.h"
#include "dol.h"
#include "ppc.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#define SSA_NO_REG      0xFFu
#define SSA_NO_VAL      0xFFFFFFFFu
#define SSA_MAX_GPR     32
#define SSA_MAX_FPR     32

/* One decoded instruction plus its SSA effects. */
typedef struct {
    uint32_t pc;
    PpcInsn  in;

    /* Register effects computed by ssa_defuse. */
    uint8_t  gpr_def;          /* GPR index written, or SSA_NO_REG   */
    uint8_t  fpr_def;          /* FPR index written, or SSA_NO_REG   */
    uint64_t gpr_uses;         /* bitmask of GPRs read               */
    uint64_t fpr_uses;         /* bitmask of FPRs read               */

    /* Constant folding (2.3). const_valid means gpr_def's SSA value
     * at this point is the 32-bit constant `const_value`. */
    uint8_t  const_valid;
    uint32_t const_value;

    /* Expression tree (2.4): a single heap-allocated string holding a
     * C-like rendering of this instruction's result. Shared via
     * ssa->expr_pool; do not free directly. NULL if not yet built. */
    const char *expr;

    /* Stack-slot reference (2.5). slot_id == SSA_NO_VAL if none. */
    uint32_t slot_id;

    /* Call-site info (2.6). Valid when in.op == PPC_OP_BL/BLA. */
    uint32_t call_site_id;
} SsaInsn;

/* Phi-function placed at the top of a block. One entry per incoming
 * block; incoming[k] is the SSA value flowing in from predecessors[k]. */
typedef struct {
    uint8_t  reg_kind;         /* 0 = GPR, 1 = FPR                   */
    uint8_t  reg;              /* register index                     */
    uint32_t block;            /* block index this phi lives in      */
} SsaPhi;

/* Block-local def/use/live summaries (2.1 / 2.2). */
typedef struct {
    size_t   first_insn;       /* index of first SsaInsn in this blk */
    size_t   insn_count;

    uint64_t gpr_defs;         /* regs written somewhere in block    */
    uint64_t fpr_defs;
    uint64_t gpr_uses_before_def;
    uint64_t fpr_uses_before_def;

    uint64_t gpr_live_in;      /* after iterative solve              */
    uint64_t fpr_live_in;

    uint64_t gpr_phi;          /* regs that need a phi here          */
    uint64_t fpr_phi;

    /* For each GPR / FPR, the SsaInsn index whose def is live at end
     * of block. SSA_NO_VAL means "incoming" (from live_in or phi). */
    uint32_t gpr_last_def[SSA_MAX_GPR];
    uint32_t fpr_last_def[SSA_MAX_FPR];
} SsaBlockInfo;

/* Stack slot recovered by 2.5. */
typedef struct {
    int32_t  offset;           /* signed displacement from r1        */
    uint8_t  width;            /* 1/2/4/8 bytes                      */
    uint8_t  is_float;
    const char *name;          /* "local_<off>" in expr_pool         */
    unsigned load_count;
    unsigned store_count;
} SsaStackSlot;

/* Call site recovered by 2.6. */
typedef struct {
    uint32_t insn_index;       /* which SsaInsn is the bl            */
    uint32_t target;           /* callee vaddr (0 if indirect)       */
    uint8_t  gpr_arg_mask;     /* bits 3..10 of the arg GPRs used    */
    uint8_t  fpr_arg_mask;     /* bits 1..8 of the arg FPRs used     */
    uint8_t  returns_gpr;      /* r3 consumed after the call         */
    uint8_t  returns_fpr;      /* f1 consumed after the call         */
} SsaCallSite;

typedef struct {
    const Cfg      *cfg;       /* not owned                          */
    const DolImage *img;       /* not owned                          */

    SsaInsn        *insns;     /* flat, in program order             */
    size_t          insn_count;

    SsaBlockInfo   *blocks;    /* one per cfg block                  */

    SsaPhi         *phis;
    size_t          phi_count;

    SsaStackSlot   *slots;
    size_t          slot_count;

    SsaCallSite    *calls;
    size_t          call_count;

    /* Bump arena for short strings (slot names, expression trees). */
    char           *expr_pool;
    size_t          expr_pool_len;
    size_t          expr_pool_cap;
} Ssa;

/* Orchestrator. Runs 2.1 -> 2.6 in order. Returns 0 on success. */
int  ssa_build(const DolImage *img, const Cfg *cfg, Ssa *out);
void ssa_free(Ssa *ssa);

/* Human-readable dump of every SSA side-table. */
void ssa_print(const Ssa *ssa, FILE *out);

#endif /* CALYNDA_DECOMPILER_CLI_SSA_H */
