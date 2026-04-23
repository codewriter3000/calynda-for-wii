/*
 * Requirement 4.4 — member offset table.
 *
 * For each class we scan every virtual method reachable through the
 * primary vtable. Each method's SysV PPC entry has `r3` = this. We
 * walk the method's raw instruction bytes looking for load/store
 * opcodes whose base register is r3 and whose immediate is a small
 * non-negative offset; the (offset, width, kind) triples become
 * candidate members.
 *
 * We stop tracking after the first unconditional branch-with-link
 * because r3 is no longer guaranteed to point at the object.
 */
#include "classes_internal.h"
#include "ppc.h"

#include <stdlib.h>
#include <string.h>

static uint8_t op_width(PpcOp op, uint8_t *kind_out) {
    switch (op) {
        case PPC_OP_LBZ: case PPC_OP_STB: *kind_out = TYPE_INT;   return 1;
        case PPC_OP_LHZ: case PPC_OP_STH: *kind_out = TYPE_INT;   return 2;
        case PPC_OP_LWZ: case PPC_OP_STW: *kind_out = TYPE_INT;   return 4;
        case PPC_OP_LFS: case PPC_OP_STFS:*kind_out = TYPE_FLOAT; return 4;
        case PPC_OP_LFD: case PPC_OP_STFD:*kind_out = TYPE_FLOAT; return 8;
        default:                                                  return 0;
    }
}

/* Scan a single method, starting at `vaddr`, bounded by `size` (or a
 * safety cap). Merge discovered fields into `ci`. */
static void scan_method(Classes *c, ClassInfo *ci,
                        uint32_t vaddr, uint32_t size) {
    if (size == 0) size = 4096;  /* fallback cap */
    if (size > 4096) size = 4096;
    uint32_t end = vaddr + size;
    for (uint32_t pc = vaddr; pc < end; pc += 4) {
        uint32_t raw;
        if (classes_read_be32(c->img, pc, &raw) != 0) break;
        PpcInsn in;
        if (ppc_decode(raw, pc, &in) != 0) break;

        /* Stop tracking at a function call; r3 may not be `this` after. */
        if (in.op == PPC_OP_BL || in.op == PPC_OP_BLA) break;
        if (in.op == PPC_OP_BLR || in.op == PPC_OP_BCTR) break;

        uint8_t kind = 0;
        uint8_t w = op_width(in.op, &kind);
        if (w == 0) continue;
        if (in.ra != 3) continue;          /* base must be r3 (this) */
        if (in.imm < 0 || in.imm > 0x4000) continue;

        ClassField f = {0};
        f.offset = (uint16_t)in.imm;
        f.width  = w;
        f.kind   = kind;
        classes_add_field(ci, f);
    }
}

/* Find the symbol covering a function vaddr to pick a size bound. */
static uint32_t method_size(const SymTable *syms, uint32_t vaddr) {
    const SymEntry *e = sym_covering(syms, vaddr);
    if (e && e->vaddr == vaddr && e->size) return e->size;
    return 0;
}

int classes_build_fields(Classes *c) {
    for (size_t i = 0; i < c->class_count; i++) {
        ClassInfo *ci = &c->classes[i];
        if (!ci->primary_vtable) continue;
        const VTable *v = classes_find_vtable(c, ci->primary_vtable);
        if (!v) continue;
        for (size_t k = 0; k < v->method_count; k++) {
            uint32_t fn = v->methods[k];
            scan_method(c, ci, fn, method_size(c->syms, fn));
        }
    }
    return 0;
}
