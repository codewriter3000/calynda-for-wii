/*
 * Shared internals for the classes module.
 * Private to src/cli/classes/.
 */
#ifndef CALYNDA_DECOMPILER_SRC_CLI_CLASSES_INTERNAL_H
#define CALYNDA_DECOMPILER_SRC_CLI_CLASSES_INTERNAL_H

#include "cli/classes.h"

/* Stage entry points. Each returns 0 on success. */
int classes_scan_vtables(Classes *c);          /* 4.1 / 4.2 */
int classes_scan_objects(Classes *c);          /* 4.1 (objects) */
int classes_build_hierarchy(Classes *c);       /* 4.3 */
int classes_build_fields(Classes *c);          /* 4.4 */
int classes_detect_mi(Classes *c);             /* 4.6 */
int classes_assign_repr(Classes *c);           /* 4.5 */

/* Helpers shared across stages. */
int  classes_is_text_vaddr(const DolImage *img, uint32_t v);
int  classes_read_be32(const DolImage *img, uint32_t v, uint32_t *out);
VTable *classes_find_vtable(Classes *c, uint32_t vaddr);
int     classes_add_field(ClassInfo *ci, ClassField f);

#endif
