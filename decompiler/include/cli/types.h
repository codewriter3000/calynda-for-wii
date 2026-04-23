/*
 * Type inference pass on top of the SSA form.
 *
 * For every SSA value (one per defining instruction), every phi, and
 * every recovered stack slot, we maintain a TypeInfo:
 *   - kind        : int / float / pointer / void / unknown
 *   - width       : 1, 2, 4, 8 bytes
 *   - signedness  : signed / unsigned / unknown
 *   - ptr_to_sym  : symbol name when this type is a pointer to a
 *                   known global (requirement 3.2)
 *   - is_array    : set when array-indexing pattern is detected (3.5)
 *   - array_stride: element stride in bytes
 *   - conflict    : set whenever two independent inference paths
 *                   disagreed and we widened to a compatible type (3.6)
 *
 * Passes (in order):
 *   3.1 types_width          — load/store opcode -> receiver width
 *   3.2 types_ptr            — const + known symbol -> pointer
 *   3.3 types_signature      — propagate from extern table (stub until
 *                              the SDK table from req 6 exists)
 *   3.4 types_sign           — load/compare opcodes -> signedness
 *   3.5 types_array          — detect array-indexing inside loops
 *   3.6 types_conflict       — merge disagreements, record casts
 */
#ifndef CALYNDA_DECOMPILER_CLI_TYPES_H
#define CALYNDA_DECOMPILER_CLI_TYPES_H

#include "cli/ssa.h"
#include "cli/symbols.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

typedef enum {
    TYPE_UNKNOWN = 0,
    TYPE_INT,
    TYPE_FLOAT,
    TYPE_PTR,
    TYPE_VOID,
} TypeKind;

typedef enum {
    SIGN_UNKNOWN = 0,
    SIGN_SIGNED,
    SIGN_UNSIGNED,
} Signedness;

typedef struct {
    TypeKind    kind;
    uint8_t     width;          /* bytes: 1, 2, 4, 8                 */
    Signedness  sign;
    const char *ptr_to_sym;     /* non-NULL => kind == TYPE_PTR      */
    uint8_t     is_array;
    uint16_t    array_stride;
    uint8_t     conflict;       /* widened due to incompatible info  */
} TypeInfo;

/* An extern signature is how requirement 3.3 consumes the RVL SDK
 * table that requirement 6 will produce. Parameters are indexed by
 * position; up to 8 GPR and 8 FPR args. */
typedef struct {
    const char *name;
    uint32_t    vaddr;
    TypeInfo    return_type;
    uint8_t     param_count;
    TypeInfo    params[16];
} TypesExternSig;

typedef struct {
    const TypesExternSig *sigs;
    size_t                count;
} TypesExternTable;

typedef struct {
    const Ssa *ssa;
    const SymTable *syms;        /* may be NULL                      */

    TypeInfo  *insn_type;        /* insn_count entries               */
    TypeInfo  *phi_type;         /* phi_count entries                */
    TypeInfo  *slot_type;        /* slot_count entries               */

    unsigned   conflicts;        /* count recorded by pass 3.6       */
} Types;

/* Primary entry point. `externs` may be NULL (fine: 3.3 no-ops). */
int  types_build(const Ssa *ssa, const SymTable *syms,
                 const TypesExternTable *externs, Types *out);
void types_free(Types *t);
void types_print(const Types *t, FILE *out);

/* Format a TypeInfo as a short C-like string into buf. Returns buf. */
const char *types_format(const TypeInfo *ti, char *buf, size_t cap);

#endif /* CALYNDA_DECOMPILER_CLI_TYPES_H */
