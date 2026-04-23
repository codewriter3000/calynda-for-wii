#include "cli/bulk.h"
#include "cli/analysis.h"

#include <stdio.h>

static int vaddr_is_text(const DolImage *img, uint32_t vaddr) {
    for (size_t i = 0; i < img->section_count; i++) {
        const DolSection *s = &img->sections[i];
        if (s->kind != DOL_SECTION_TEXT) continue;
        if (vaddr >= s->virtual_addr && vaddr < s->virtual_addr + s->size)
            return 1;
    }
    return 0;
}

int emit_all_functions(const DolImage *img, const SymTable *syms, FILE *out) {
    if (!img || !syms || !out) return 1;
    if (syms->count == 0) {
        fprintf(stderr, "error: --disasm-all-functions requires --symbols\n");
        return 1;
    }
    fprintf(out, "# all text functions with known size from symbol table\n");
    size_t emitted = 0;
    for (size_t i = 0; i < syms->count; i++) {
        const SymEntry *e = &syms->entries[i];
        if (e->size == 0) continue;
        if (!vaddr_is_text(img, e->vaddr)) continue;
        size_t rem = 0;
        const uint8_t *p = dol_vaddr_to_ptr(img, e->vaddr, &rem);
        if (!p) continue;
        unsigned count = e->size / 4u;
        if ((size_t)count * 4u > rem) count = (unsigned)(rem / 4u);
        if (count == 0) continue;
        fprintf(out, "\n%s:  # 0x%08x, %u insns\n",
                e->name, e->vaddr, count);
        emit_disassembly_range(out, p, e->vaddr, count, syms);
        emitted++;
    }
    fprintf(stderr, "emitted %zu functions\n", emitted);
    return 0;
}
