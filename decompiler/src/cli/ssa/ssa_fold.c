/*
 * Requirement 2.3 — fold the standard Wii prologue pattern
 *     addis rX, 0, HI
 *     addi  rX, rX, LO        (or ori rX, rX, LO)
 * into a single 32-bit constant attached to the second instruction.
 *
 * We also tag a lone `addi rX, 0, imm` and `addis rX, 0, imm` with
 * their constant value so downstream passes can recognize them.
 *
 * Shadows are reset at every basic-block boundary. That loses some
 * cross-block propagation opportunities but keeps the pass trivially
 * correct: inside a block the program order matches SSA order.
 */
#include "ssa_internal.h"

#include <string.h>

typedef struct {
    uint32_t value;
    uint8_t  valid;
} GprShadow;

int ssa_fold_prologue(Ssa *ssa) {
    for (size_t bi = 0; bi < ssa->cfg->block_count; bi++) {
        SsaBlockInfo *b = &ssa->blocks[bi];
        GprShadow sh[SSA_MAX_GPR];
        memset(sh, 0, sizeof(sh));

        for (size_t k = 0; k < b->insn_count; k++) {
            SsaInsn *si = &ssa->insns[b->first_insn + k];
            const PpcInsn *in = &si->in;

            switch (in->op) {
                case PPC_OP_ADDIS: {
                    uint32_t v = (uint32_t)((int32_t)in->imm << 16);
                    if (in->ra == 0) {
                        sh[in->rd].valid = 1;
                        sh[in->rd].value = v;
                        si->const_valid = 1;
                        si->const_value = v;
                    } else if (sh[in->ra].valid) {
                        sh[in->rd].valid = 1;
                        sh[in->rd].value = sh[in->ra].value + v;
                        si->const_valid = 1;
                        si->const_value = sh[in->rd].value;
                    } else {
                        sh[in->rd].valid = 0;
                    }
                    break;
                }
                case PPC_OP_ADDI: {
                    int32_t lo = in->imm;  /* sign-extended */
                    if (in->ra == 0) {
                        sh[in->rd].valid = 1;
                        sh[in->rd].value = (uint32_t)lo;
                        si->const_valid = 1;
                        si->const_value = (uint32_t)lo;
                    } else if (sh[in->ra].valid) {
                        sh[in->rd].valid = 1;
                        sh[in->rd].value = sh[in->ra].value + (uint32_t)lo;
                        si->const_valid = 1;
                        si->const_value = sh[in->rd].value;
                    } else {
                        sh[in->rd].valid = 0;
                    }
                    break;
                }
                case PPC_OP_ORI: {
                    /* ori rA, rS, imm : dest = ra, src = rd */
                    if (sh[in->rd].valid) {
                        uint32_t v = sh[in->rd].value | (uint32_t)in->imm;
                        sh[in->ra].valid = 1;
                        sh[in->ra].value = v;
                        si->const_valid = 1;
                        si->const_value = v;
                    } else {
                        sh[in->ra].valid = 0;
                    }
                    break;
                }
                default:
                    /* Any other defining instruction clears shadow. */
                    if (si->gpr_def < SSA_MAX_GPR)
                        sh[si->gpr_def].valid = 0;
                    break;
            }
        }
    }
    return 0;
}
