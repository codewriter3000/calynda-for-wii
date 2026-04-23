#include "cli/bulk.h"
#include "ppc.h"

#include <inttypes.h>
#include <stdio.h>

static int sym_in_text(const DolImage *img, uint32_t vaddr, uint32_t size) {
    for (size_t i = 0; i < img->section_count; i++) {
        const DolSection *s = &img->sections[i];
        if (s->kind != DOL_SECTION_TEXT) continue;
        uint32_t end = s->virtual_addr + s->size;
        if (vaddr >= s->virtual_addr && vaddr + size <= end) return 1;
    }
    return 0;
}

static uint64_t total_text(const DolImage *img) {
    uint64_t t = 0;
    for (size_t i = 0; i < img->section_count; i++) {
        const DolSection *s = &img->sections[i];
        if (s->kind == DOL_SECTION_TEXT) t += s->size;
    }
    return t;
}

static uint64_t count_unknowns(const DolImage *img) {
    uint64_t u = 0;
    for (size_t i = 0; i < img->section_count; i++) {
        const DolSection *s = &img->sections[i];
        if (s->kind != DOL_SECTION_TEXT) continue;
        size_t rem = 0;
        const uint8_t *p = dol_vaddr_to_ptr(img, s->virtual_addr, &rem);
        if (!p) continue;
        unsigned n = (unsigned)(rem / 4u);
        for (unsigned k = 0; k < n; k++) {
            uint32_t w = ((uint32_t)p[k*4] << 24) | ((uint32_t)p[k*4+1] << 16) |
                         ((uint32_t)p[k*4+2] <<  8) |  (uint32_t)p[k*4+3];
            PpcInsn in;
            ppc_decode(w, s->virtual_addr + k*4u, &in);
            if (in.op == PPC_OP_UNKNOWN) u++;
        }
    }
    return u;
}

int emit_stats(const DolImage *img, const SymTable *syms, FILE *out) {
    if (!img || !out) return 1;
    uint64_t text_bytes = total_text(img);
    uint64_t covered = 0, sized_syms = 0;
    if (syms) {
        for (size_t i = 0; i < syms->count; i++) {
            const SymEntry *e = &syms->entries[i];
            if (e->size == 0) continue;
            if (!sym_in_text(img, e->vaddr, e->size)) continue;
            covered += e->size;
            sized_syms++;
        }
    }
    uint64_t unknowns = count_unknowns(img);
    uint64_t total_insns = text_bytes / 4u;

    fprintf(out, "stats:\n");
    fprintf(out, "  text bytes:          %" PRIu64 "\n", text_bytes);
    fprintf(out, "  text instructions:   %" PRIu64 "\n", total_insns);
    fprintf(out, "  decoded unknowns:    %" PRIu64 " (%.2f%%)\n",
            unknowns,
            total_insns ? 100.0 * (double)unknowns / (double)total_insns : 0.0);
    fprintf(out, "  sized symbols:       %" PRIu64 "\n", sized_syms);
    fprintf(out, "  symbol-covered text: %" PRIu64 " bytes (%.2f%%)\n",
            covered,
            text_bytes ? 100.0 * (double)covered / (double)text_bytes : 0.0);

    if (sized_syms > 1) {
        uint32_t prev_end = 0;
        int have_prev = 0;
        uint32_t gap_addr = 0, gap_size = 0;
        fprintf(out, "  largest gaps (up to 5):\n");
        uint32_t top_addr[5] = {0}, top_size[5] = {0};
        for (size_t i = 0; i < syms->count; i++) {
            const SymEntry *e = &syms->entries[i];
            if (e->size == 0) continue;
            if (!sym_in_text(img, e->vaddr, e->size)) continue;
            if (have_prev && e->vaddr > prev_end) {
                gap_addr = prev_end;
                gap_size = e->vaddr - prev_end;
                for (int j = 0; j < 5; j++) {
                    if (gap_size > top_size[j]) {
                        for (int k = 4; k > j; k--) {
                            top_size[k] = top_size[k-1];
                            top_addr[k] = top_addr[k-1];
                        }
                        top_size[j] = gap_size;
                        top_addr[j] = gap_addr;
                        break;
                    }
                }
            }
            prev_end = e->vaddr + e->size;
            have_prev = 1;
        }
        for (int j = 0; j < 5 && top_size[j]; j++) {
            fprintf(out, "    0x%08x  %u bytes\n",
                    top_addr[j], top_size[j]);
        }
    }
    return 0;
}
