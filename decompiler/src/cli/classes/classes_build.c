/*
 * Classes module orchestrator + helpers + pretty-printer.
 */
#include "classes_internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int classes_is_text_vaddr(const DolImage *img, uint32_t v) {
    const DolSection *s = dol_section_for_vaddr(img, v);
    return s && s->kind == DOL_SECTION_TEXT;
}

int classes_read_be32(const DolImage *img, uint32_t v, uint32_t *out) {
    size_t rem = 0;
    const uint8_t *p = dol_vaddr_to_ptr(img, v, &rem);
    if (!p || rem < 4) return -1;
    *out = ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] <<  8) |  (uint32_t)p[3];
    return 0;
}

VTable *classes_find_vtable(Classes *c, uint32_t vaddr) {
    for (size_t i = 0; i < c->vtable_count; i++)
        if (c->vtables[i].vaddr == vaddr) return &c->vtables[i];
    return NULL;
}

int classes_add_field(ClassInfo *ci, ClassField f) {
    for (size_t i = 0; i < ci->field_count; i++) {
        if (ci->fields[i].offset == f.offset) {
            /* Widen to the larger of the two widths. */
            if (f.width > ci->fields[i].width) ci->fields[i].width = f.width;
            if (ci->fields[i].kind == 0) ci->fields[i].kind = f.kind;
            return 0;
        }
    }
    size_t n = ci->field_count;
    ClassField *tmp = (ClassField *)realloc(ci->fields,
                                            (n + 1) * sizeof(*tmp));
    if (!tmp) return -1;
    ci->fields = tmp;
    ci->fields[n] = f;
    ci->field_count = n + 1;
    return 0;
}

const char *classes_repr_name(ClassRepr r) {
    switch (r) {
        case CLASS_REPR_EXTERN:  return "extern";
        case CLASS_REPR_BINDING: return "binding";
        case CLASS_REPR_UNION:   return "union";
        default:                 return "unknown";
    }
}

int classes_build(const DolImage *img, const SymTable *syms, Classes *out) {
    if (!img || !syms || !out) return -1;
    memset(out, 0, sizeof(*out));
    out->img = img;
    out->syms = syms;
    if (classes_scan_vtables(out)    != 0) { classes_free(out); return -1; }
    if (classes_scan_objects(out)    != 0) { classes_free(out); return -1; }
    if (classes_build_hierarchy(out) != 0) { classes_free(out); return -1; }
    if (classes_detect_mi(out)       != 0) { classes_free(out); return -1; }
    if (classes_build_fields(out)    != 0) { classes_free(out); return -1; }
    if (classes_assign_repr(out)     != 0) { classes_free(out); return -1; }
    return 0;
}

void classes_free(Classes *c) {
    if (!c) return;
    for (size_t i = 0; i < c->vtable_count; i++) free(c->vtables[i].methods);
    free(c->vtables);
    for (size_t i = 0; i < c->class_count; i++) free(c->classes[i].fields);
    free(c->classes);
    memset(c, 0, sizeof(*c));
}

static const char *kind_name(uint8_t k) {
    switch (k) {
        case TYPE_INT:   return "int";
        case TYPE_FLOAT: return "float";
        case TYPE_PTR:   return "ptr";
        case TYPE_VOID:  return "void";
        default:         return "?";
    }
}

void classes_print(const Classes *c, FILE *out) {
    fprintf(out, "# Classes: %zu vtables, %zu classes\n",
            c->vtable_count, c->class_count);
    for (size_t i = 0; i < c->vtable_count; i++) {
        const VTable *v = &c->vtables[i];
        fprintf(out, "vtable 0x%08x %s  methods=%zu",
                v->vaddr, v->name ? v->name : "(anon)", v->method_count);
        if (v->parent_vtable)
            fprintf(out, "  parent=0x%08x", v->parent_vtable);
        fputc('\n', out);
        for (size_t k = 0; k < v->method_count; k++) {
            const char *nm = sym_lookup(c->syms, v->methods[k]);
            fprintf(out, "  [%zu] 0x%08x %s\n", k, v->methods[k],
                    nm ? nm : "");
        }
    }
    for (size_t i = 0; i < c->class_count; i++) {
        const ClassInfo *ci = &c->classes[i];
        fprintf(out, "class 0x%08x %s  repr=%s",
                ci->vaddr, ci->name ? ci->name : "(anon)",
                classes_repr_name(ci->repr));
        if (ci->primary_vtable)
            fprintf(out, "  vtable=0x%08x", ci->primary_vtable);
        if (ci->parent_class)
            fprintf(out, "  parent=0x%08x", ci->parent_class);
        if (ci->secondary_vtable_off)
            fprintf(out, "  mixin@+%u=0x%08x",
                    ci->secondary_vtable_off, ci->secondary_vtable);
        fputc('\n', out);
        for (size_t k = 0; k < ci->field_count; k++) {
            const ClassField *f = &ci->fields[k];
            fprintf(out, "  +%-4u %s%u%s\n",
                    f->offset, kind_name(f->kind),
                    f->width ? f->width * 8u : 0u,
                    f->is_vtable_ptr ? "  (vtable)" : "");
        }
    }
}
