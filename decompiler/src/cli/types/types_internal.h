/*
 * Shared internals for the types module. Private to src/cli/types/.
 */
#ifndef CALYNDA_DECOMPILER_SRC_CLI_TYPES_INTERNAL_H
#define CALYNDA_DECOMPILER_SRC_CLI_TYPES_INTERNAL_H

#include "cli/types.h"

/* Pass entry points. */
int types_pass_width(Types *t);          /* 3.1 */
int types_pass_ptr(Types *t);            /* 3.2 */
int types_pass_signature(Types *t,
                         const TypesExternTable *tab); /* 3.3 */
int types_pass_sign(Types *t);           /* 3.4 */
int types_pass_array(Types *t);          /* 3.5 */
int types_pass_conflict(Types *t);       /* 3.6 */

/* Merge `src` into `*dst` in-place, widening where the two disagree
 * and setting conflict on true disagreements. Returns 1 if *dst was
 * changed, 0 otherwise. Used by 3.6 and by the forward propagation
 * of every other pass. */
int types_merge(TypeInfo *dst, const TypeInfo src);

#endif
