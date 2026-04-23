/*
 * Tests for the classes module (requirement 4).
 *
 * We synthesize a minimal DOL with one text section holding two tiny
 * "methods" and one data section holding a vtable + class instance
 * + a derived vtable. Then we run classes_build and assert on the
 * resulting vtable layout, hierarchy, fields and representation.
 */
#include "cli/classes.h"
#include "cli/symbols.h"
#include "dol.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void wbe32(uint8_t *p, uint32_t v) {
    p[0] = v >> 24; p[1] = v >> 16; p[2] = v >> 8; p[3] = v;
}

/* Layout:
 *   text0 @ 0x80004000: 0x40 bytes of blr/lwz-derived methods
 *   data0 @ 0x80006000: 16 bytes  = vtableA (4 methods)
 *   data1 @ 0x80006010: 12 bytes  = vtableB (derived, 3 methods)
 *   data2 @ 0x80006020: 8 bytes   = instanceA (points at vtableA)
 *   data3 @ 0x80006028: 8 bytes   = instanceB (points at vtableB)
 */
#define TXT_VA 0x80004000u
#define DAT_VA 0x80006000u
#define FN_A   (TXT_VA + 0x00)
#define FN_B   (TXT_VA + 0x10)
#define FN_C   (TXT_VA + 0x20)
#define FN_D   (TXT_VA + 0x30)
#define VT_A   (DAT_VA + 0x00)
#define VT_B   (DAT_VA + 0x10)
#define OBJ_A  (DAT_VA + 0x20)
#define OBJ_B  (DAT_VA + 0x28)

static uint8_t *build_dol(size_t *out_size) {
    /* DOL header is 0x100 bytes. Put text at file offset 0x100 (64
     * bytes) then data at 0x140 (48 bytes). */
    size_t txt_sz = 0x40;
    size_t dat_sz = 0x30;
    size_t total = 0x100 + txt_sz + dat_sz;
    uint8_t *buf = calloc(total, 1);
    /* text0 */
    wbe32(buf + 0x00, 0x100);          /* file offset           */
    wbe32(buf + 0x48, TXT_VA);         /* vaddr                 */
    wbe32(buf + 0x90, (uint32_t)txt_sz);
    /* data0 */
    wbe32(buf + 0x1C, 0x100 + (uint32_t)txt_sz);
    wbe32(buf + 0x64, DAT_VA);
    wbe32(buf + 0xAC, (uint32_t)dat_sz);
    /* entrypoint */
    wbe32(buf + 0xE0, TXT_VA);

    /* text: we only need FN_A..FN_D to decode something with ra=3
     * for fields. Use:
     *   FN_A: lwz r4, 0(r3)   ; stw r4, 8(r3)   ; blr
     *   FN_B: lbz r5, 12(r3)  ; blr
     *   FN_C: lfs f1, 16(r3)  ; blr
     *   FN_D: blr
     */
    uint8_t *tx = buf + 0x100;
    /* encoders */
    #define DFORM(opc, rd, ra, d) (((opc)<<26)|((rd)<<21)|((ra)<<16)| \
                                   ((uint16_t)(d)))
    wbe32(tx + 0x00, DFORM(32, 4, 3, 0));
    wbe32(tx + 0x04, DFORM(36, 4, 3, 8));
    wbe32(tx + 0x08, 0x4E800020u);
    wbe32(tx + 0x0C, 0x60000000u);

    wbe32(tx + 0x10, DFORM(34, 5, 3, 12));
    wbe32(tx + 0x14, 0x4E800020u);
    wbe32(tx + 0x18, 0x60000000u);
    wbe32(tx + 0x1C, 0x60000000u);

    wbe32(tx + 0x20, DFORM(48, 1, 3, 16));
    wbe32(tx + 0x24, 0x4E800020u);
    wbe32(tx + 0x28, 0x60000000u);
    wbe32(tx + 0x2C, 0x60000000u);

    wbe32(tx + 0x30, 0x4E800020u);
    wbe32(tx + 0x34, 0x60000000u);
    wbe32(tx + 0x38, 0x60000000u);
    wbe32(tx + 0x3C, 0x60000000u);
    #undef DFORM

    /* data: vtableA @+0, vtableB @+0x10, objA @+0x20, objB @+0x28 */
    uint8_t *da = buf + 0x100 + txt_sz;
    wbe32(da + 0x00, FN_A);
    wbe32(da + 0x04, FN_B);
    wbe32(da + 0x08, 0);     /* padding — vtable A has 2 methods */
    wbe32(da + 0x0C, 0);

    wbe32(da + 0x10, FN_A);  /* VB is derived: first 2 match VA  */
    wbe32(da + 0x14, FN_B);
    wbe32(da + 0x18, FN_C);  /* plus two new virtual methods     */

    wbe32(da + 0x20, VT_A);
    wbe32(da + 0x24, 0);

    wbe32(da + 0x28, VT_B);
    wbe32(da + 0x2C, 0);

    *out_size = total;
    return buf;
}

static void add_sym(SymTable *st, uint32_t vaddr, uint32_t size,
                    const char *name) {
    if (st->count == st->cap) {
        size_t nc = st->cap ? st->cap * 2 : 8;
        st->entries = realloc(st->entries, nc * sizeof(SymEntry));
        st->cap = nc;
    }
    st->entries[st->count].vaddr = vaddr;
    st->entries[st->count].size  = size;
    strncpy(st->entries[st->count].name, name, SYM_NAME_MAX - 1);
    st->entries[st->count].name[SYM_NAME_MAX - 1] = '\0';
    st->count++;
}

static int cmp_sym(const void *a, const void *b) {
    const SymEntry *ea = a, *eb = b;
    if (ea->vaddr < eb->vaddr) return -1;
    if (ea->vaddr > eb->vaddr) return  1;
    return 0;
}

int main(void) {
    size_t sz;
    uint8_t *bytes = build_dol(&sz);

    DolImage img;
    assert(dol_parse(bytes, sz, &img) == DOL_OK);

    SymTable st; memset(&st, 0, sizeof(st));
    add_sym(&st, FN_A,  0x10, "FnA");
    add_sym(&st, FN_B,  0x10, "FnB");
    add_sym(&st, FN_C,  0x10, "FnC");
    add_sym(&st, FN_D,  0x10, "FnD");
    add_sym(&st, VT_A,  0x10, "VtA");
    add_sym(&st, VT_B,  0x10, "VtB");
    add_sym(&st, OBJ_A, 0x08, "ObjA");
    add_sym(&st, OBJ_B, 0x08, "ObjB");
    qsort(st.entries, st.count, sizeof(SymEntry), cmp_sym);

    Classes cl;
    assert(classes_build(&img, &st, &cl) == 0);

    /* 4.1 / 4.2 — we should have 2 vtables and 2 classes. */
    assert(cl.vtable_count == 2);
    assert(cl.class_count  == 2);

    /* Locate them by name. */
    VTable *va = NULL, *vb = NULL;
    for (size_t i = 0; i < cl.vtable_count; i++) {
        if (strcmp(cl.vtables[i].name, "VtA") == 0) va = &cl.vtables[i];
        if (strcmp(cl.vtables[i].name, "VtB") == 0) vb = &cl.vtables[i];
    }
    assert(va && vb);
    assert(va->method_count == 2);
    assert(vb->method_count >= 3);
    assert(va->methods[0] == FN_A);
    assert(vb->methods[0] == FN_A);
    assert(vb->methods[2] == FN_C);

    /* 4.3 — vb should inherit from va (first 2 entries match). */
    assert(vb->parent_vtable == VT_A);

    /* Classes wired to their vtables. */
    ClassInfo *ca = NULL, *cb = NULL;
    for (size_t i = 0; i < cl.class_count; i++) {
        if (strcmp(cl.classes[i].name, "ObjA") == 0) ca = &cl.classes[i];
        if (strcmp(cl.classes[i].name, "ObjB") == 0) cb = &cl.classes[i];
    }
    assert(ca && cb);
    assert(ca->primary_vtable == VT_A);
    assert(cb->primary_vtable == VT_B);
    assert(cb->parent_class   == OBJ_A);

    /* 4.4 — FnA does lwz r4,0(r3) and stw r4,8(r3); FnB does lbz
     * r5,12(r3); FnC does lfs f1,16(r3). ObjA uses VtA=[FnA,FnB] so
     * it should see offsets 0, 8, 12. ObjB uses VtB=[FnA,FnB,FnC] so
     * it should also pick up offset 16. */
    int a_off0 = 0, a_off8 = 0, a_off12 = 0;
    for (size_t i = 0; i < ca->field_count; i++) {
        ClassField *f = &ca->fields[i];
        if (f->offset == 0  && f->is_vtable_ptr) a_off0 = 1;
        if (f->offset == 8  && f->width == 4)    a_off8 = 1;
        if (f->offset == 12 && f->width == 1)    a_off12 = 1;
    }
    assert(a_off0);
    assert(a_off8);
    assert(a_off12);

    int b_off16 = 0;
    for (size_t i = 0; i < cb->field_count; i++)
        if (cb->fields[i].offset == 16 && cb->fields[i].kind == TYPE_FLOAT)
            b_off16 = 1;
    assert(b_off16);

    /* 4.5 — polymorphic classes map to UNION. */
    assert(ca->repr == CLASS_REPR_UNION);
    assert(cb->repr == CLASS_REPR_UNION);

    puts("-- all classes tests passed --");

    classes_free(&cl);
    free(st.entries);
    free(bytes);
    return 0;
}
