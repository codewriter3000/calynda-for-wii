/*
 * Requirement 4.3 — class hierarchy reconstruction.
 *
 * For every pair (A, B) of vtables, if B's first |A.methods| entries
 * are identical to A's, we infer B derives from A. The longest such A
 * wins (closest ancestor). We record this as `parent_vtable` on B and
 * propagate `parent_class` to all classes pointing at that vtable.
 */
#include "classes_internal.h"

#include <string.h>

static int is_prefix(const VTable *parent, const VTable *child) {
    if (parent->method_count == 0) return 0;
    if (parent->method_count > child->method_count) return 0;
    for (size_t i = 0; i < parent->method_count; i++) {
        if (parent->methods[i] != child->methods[i]) return 0;
    }
    return 1;
}

int classes_build_hierarchy(Classes *c) {
    for (size_t i = 0; i < c->vtable_count; i++) {
        VTable *child = &c->vtables[i];
        const VTable *best = NULL;
        for (size_t j = 0; j < c->vtable_count; j++) {
            if (i == j) continue;
            const VTable *cand = &c->vtables[j];
            if (cand->method_count >= child->method_count) continue;
            if (!is_prefix(cand, child)) continue;
            if (!best || cand->method_count > best->method_count)
                best = cand;
        }
        child->parent_vtable = best ? best->vaddr : 0;
    }

    /* Promote parent_vtable into parent_class links for each object. */
    for (size_t i = 0; i < c->class_count; i++) {
        ClassInfo *ci = &c->classes[i];
        if (!ci->primary_vtable) continue;
        VTable *v = classes_find_vtable(c, ci->primary_vtable);
        if (!v || !v->parent_vtable) continue;
        for (size_t j = 0; j < c->class_count; j++) {
            if (i == j) continue;
            if (c->classes[j].primary_vtable == v->parent_vtable) {
                ci->parent_class = c->classes[j].vaddr;
                break;
            }
        }
    }
    return 0;
}

/*
 * Requirement 4.6 — detect secondary-vtable mixins.
 *
 * CodeWarrior emits, for each multiply-inherited base, a second
 * vtable pointer at a non-zero offset inside the object. We scan the
 * class symbol's entire memory range for a word that matches another
 * known vtable's address; the first match at offset != 0 is recorded
 * as the mixin.
 */
int classes_detect_mi(Classes *c) {
    for (size_t i = 0; i < c->class_count; i++) {
        ClassInfo *ci = &c->classes[i];
        const SymEntry *e = NULL;
        for (size_t k = 0; k < c->syms->count; k++) {
            if (c->syms->entries[k].vaddr == ci->vaddr) {
                e = &c->syms->entries[k];
                break;
            }
        }
        if (!e || e->size < 8) continue;
        size_t words = e->size / 4u;
        for (size_t w = 1; w < words && w < 64; w++) {
            uint32_t val;
            if (classes_read_be32(c->img, ci->vaddr + (uint32_t)(w * 4),
                                  &val) != 0) break;
            if (val == ci->primary_vtable) continue;
            if (classes_find_vtable(c, val)) {
                ci->secondary_vtable_off = (uint16_t)(w * 4);
                ci->secondary_vtable     = val;
                /* Record the mixin as a field. */
                ClassField f = {0};
                f.offset = ci->secondary_vtable_off;
                f.width  = 4;
                f.kind   = TYPE_PTR;
                f.is_vtable_ptr = 1;
                f.vtable_vaddr  = val;
                classes_add_field(ci, f);
                break;
            }
        }
    }
    return 0;
}
