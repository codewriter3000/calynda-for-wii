#include "cli/bulk.h"

#include <stdio.h>
#include <string.h>

int emit_diff(const DolImage *a, const DolImage *b,
              const SymTable *syms, FILE *out) {
    if (!a || !b || !out) return 1;
    if (!syms || syms->count == 0) {
        fprintf(stderr, "error: --diff requires --symbols\n");
        return 1;
    }
    size_t compared = 0, differing = 0, only_a = 0, only_b = 0;
    for (size_t i = 0; i < syms->count; i++) {
        const SymEntry *e = &syms->entries[i];
        if (e->size == 0) continue;
        size_t ra = 0, rb = 0;
        const uint8_t *pa = dol_vaddr_to_ptr(a, e->vaddr, &ra);
        const uint8_t *pb = dol_vaddr_to_ptr(b, e->vaddr, &rb);
        if (!pa && !pb) continue;
        if (!pa) { only_b++; continue; }
        if (!pb) { only_a++; continue; }
        size_t n = e->size;
        if (n > ra) n = ra;
        if (n > rb) n = rb;
        if (n == 0) continue;
        compared++;
        if (memcmp(pa, pb, n) == 0) continue;
        differing++;
        size_t off = 0;
        while (off < n && pa[off] == pb[off]) off++;
        fprintf(out, "%-40s  0x%08x  first diff +0x%zx: %02x%02x%02x%02x"
                     " -> %02x%02x%02x%02x\n",
                e->name, e->vaddr, off,
                pa[off],   off+1 < n ? pa[off+1] : 0,
                off+2 < n ? pa[off+2] : 0, off+3 < n ? pa[off+3] : 0,
                pb[off],   off+1 < n ? pb[off+1] : 0,
                off+2 < n ? pb[off+2] : 0, off+3 < n ? pb[off+3] : 0);
    }
    fprintf(stderr,
            "diff: compared=%zu  differing=%zu  only-in-a=%zu  only-in-b=%zu\n",
            compared, differing, only_a, only_b);
    return 0;
}
