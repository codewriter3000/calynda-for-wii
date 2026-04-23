/*
 * Class / vtable reconstruction (requirement 4).
 *
 *   4.1 Vtable pointer identification — find data words at symbol
 *       offsets that point into a function-pointer table, mark the
 *       object at that address as having a vtable at offset 0.
 *   4.2 Vtable layout extraction — for each vtable, record the
 *       ordered list of virtual method addresses.
 *   4.3 Class hierarchy reconstruction — when one vtable's first N
 *       entries are a prefix of another vtable's, infer inheritance.
 *   4.4 Member offset table — build offset -> (width, kind) maps by
 *       scanning the class's virtual methods for `*(rX, N(r3))`
 *       accesses.
 *   4.5 Calynda representation mapping — decide whether each class
 *       is a union (polymorphic), a flat binding (plain struct),
 *       or extern (opaque).
 *   4.6 Multiple-inheritance / mixin handling — detect the pattern
 *       of a secondary vtable pointer at a non-zero offset and
 *       record it as a nested mixin field.
 */
#ifndef CALYNDA_DECOMPILER_CLI_CLASSES_H
#define CALYNDA_DECOMPILER_CLI_CLASSES_H

#include "cli/symbols.h"
#include "cli/types.h"
#include "dol.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

typedef enum {
    CLASS_REPR_UNKNOWN = 0,
    CLASS_REPR_EXTERN,        /* opaque, never dereferenced        */
    CLASS_REPR_BINDING,       /* flat struct / POD                 */
    CLASS_REPR_UNION,         /* has virtual methods (polymorphic) */
} ClassRepr;

typedef struct {
    uint16_t  offset;         /* byte offset from start of object  */
    uint8_t   width;          /* 1 / 2 / 4 / 8                     */
    uint8_t   kind;           /* one of TypeKind                   */
    uint8_t   is_vtable_ptr;  /* this field is the vtable itself   */
    uint32_t  vtable_vaddr;   /* non-zero when is_vtable_ptr       */
} ClassField;

typedef struct {
    uint32_t    vaddr;        /* vtable symbol address              */
    const char *name;         /* from SymTable, or NULL             */
    uint32_t   *methods;      /* heap-allocated list of func vaddrs */
    size_t      method_count;
    uint32_t    parent_vtable;/* 0 if none                          */
} VTable;

typedef struct {
    uint32_t    vaddr;        /* instance or class symbol address   */
    const char *name;
    uint32_t    primary_vtable;       /* at offset 0, or 0 if none */
    uint16_t    secondary_vtable_off; /* non-zero => MI            */
    uint32_t    secondary_vtable;     /* 0 if none                 */
    ClassField *fields;
    size_t      field_count;
    uint32_t    parent_class;         /* class vaddr, 0 if none    */
    ClassRepr   repr;
} ClassInfo;

typedef struct {
    const DolImage *img;
    const SymTable *syms;
    VTable        *vtables;
    size_t         vtable_count;
    ClassInfo     *classes;
    size_t         class_count;
} Classes;

int  classes_build(const DolImage *img, const SymTable *syms,
                   Classes *out);
void classes_free(Classes *c);
void classes_print(const Classes *c, FILE *out);

/* Name of a class representation (for the pretty printer). */
const char *classes_repr_name(ClassRepr r);

#endif /* CALYNDA_DECOMPILER_CLI_CLASSES_H */
