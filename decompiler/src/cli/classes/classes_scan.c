/*
 * Requirements 4.1 / 4.2 — vtable identification and layout
 * extraction.
 *
 * A vtable is a sequence of 4-byte words in a data/rodata section in
 * which every word is the address of a text-section function. We
 * consider a symbol to be a vtable when it starts with at least two
 * consecutive text pointers.
 *
 * In CodeWarrior output, the vtable sits in `.rodata` and the data
 * symbol for an *instance* starts with a 4-byte field whose value is
 * the vtable's vaddr. We detect both shapes:
 *
 *   - first word is a text pointer         -> this symbol *is* a vtable
 *   - first word is a known vtable address -> this symbol is a class
 *                                              instance (has a vtable
 *                                              at offset 0)
 */
#include "classes_internal.h"

#include <stdlib.h>
#include <string.h>

/* Return the number of consecutive text pointers starting at `vaddr`,
 * capped at `cap` to avoid running into the next symbol. */
static size_t count_consecutive_text_ptrs(const DolImage *img,
                                          uint32_t vaddr, size_t cap) {
    size_t n = 0;
    for (size_t i = 0; i < cap; i++) {
        uint32_t w;
        if (classes_read_be32(img, vaddr + (uint32_t)(i * 4), &w) != 0) break;
        if (!classes_is_text_vaddr(img, w)) break;
        n++;
    }
    return n;
}

static int push_vtable(Classes *c, const SymEntry *e, size_t n_methods) {
    size_t idx = c->vtable_count;
    VTable *tmp = (VTable *)realloc(c->vtables,
                                    (idx + 1) * sizeof(*tmp));
    if (!tmp) return -1;
    c->vtables = tmp;
    VTable *v = &c->vtables[idx];
    memset(v, 0, sizeof(*v));
    v->vaddr = e->vaddr;
    v->name  = e->name;
    v->method_count = n_methods;
    v->methods = (uint32_t *)calloc(n_methods, sizeof(uint32_t));
    if (!v->methods) return -1;
    for (size_t k = 0; k < n_methods; k++) {
        if (classes_read_be32(c->img, e->vaddr + (uint32_t)(k * 4),
                              &v->methods[k]) != 0) return -1;
    }
    c->vtable_count = idx + 1;
    return 0;
}

int classes_scan_vtables(Classes *c) {
    for (size_t i = 0; i < c->syms->count; i++) {
        const SymEntry *e = &c->syms->entries[i];
        const DolSection *s = dol_section_for_vaddr(c->img, e->vaddr);
        if (!s || s->kind != DOL_SECTION_DATA) continue;
        size_t cap = (e->size ? e->size : 16u) / 4u;
        if (cap < 2) continue;
        size_t n = count_consecutive_text_ptrs(c->img, e->vaddr, cap);
        /* Require at least 2 text pointers to call something a vtable
         * (a single text pointer could be a plain function reference). */
        if (n >= 2) {
            if (push_vtable(c, e, n) != 0) return -1;
        }
    }
    return 0;
}

static int push_class(Classes *c, const SymEntry *e,
                      uint32_t vtable_vaddr) {
    size_t idx = c->class_count;
    ClassInfo *tmp = (ClassInfo *)realloc(c->classes,
                                          (idx + 1) * sizeof(*tmp));
    if (!tmp) return -1;
    c->classes = tmp;
    ClassInfo *ci = &c->classes[idx];
    memset(ci, 0, sizeof(*ci));
    ci->vaddr = e->vaddr;
    ci->name  = e->name;
    ci->primary_vtable = vtable_vaddr;
    c->class_count = idx + 1;

    /* Seed the field table with the vtable pointer at +0. */
    ClassField f = {0};
    f.offset = 0;
    f.width  = 4;
    f.kind   = TYPE_PTR;
    f.is_vtable_ptr = 1;
    f.vtable_vaddr  = vtable_vaddr;
    return classes_add_field(ci, f);
}

int classes_scan_objects(Classes *c) {
    for (size_t i = 0; i < c->syms->count; i++) {
        const SymEntry *e = &c->syms->entries[i];
        const DolSection *s = dol_section_for_vaddr(c->img, e->vaddr);
        if (!s || s->kind != DOL_SECTION_DATA) continue;
        /* Skip symbols that are themselves vtables. */
        if (classes_find_vtable(c, e->vaddr)) continue;
        uint32_t w;
        if (classes_read_be32(c->img, e->vaddr, &w) != 0) continue;
        if (!classes_find_vtable(c, w)) continue;
        if (push_class(c, e, w) != 0) return -1;
    }
    return 0;
}
