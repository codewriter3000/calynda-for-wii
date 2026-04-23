#include "cli/analysis.h"

#include <stdio.h>
#include <stdlib.h>

void dump_data(const DolImage *img, const SymTable *syms, FILE *out) {
    fprintf(out, "\ndata-section hex dump:\n");
    for (size_t i = 0; i < img->section_count; i++) {
        const DolSection *s = &img->sections[i];
        if (s->kind != DOL_SECTION_DATA) continue;
        fprintf(out, "\ndata%u @ 0x%08x  size=0x%x:\n",
                s->index, s->virtual_addr, s->size);
        const uint8_t *data = img->bytes + s->file_offset;
        for (uint32_t off = 0; off < s->size; off += 16u) {
            uint32_t row_vaddr = s->virtual_addr + off;
            const char *sym = sym_lookup(syms, row_vaddr);
            if (sym) fprintf(out, "\n<%s>:\n", sym);
            fprintf(out, "  0x%08x: ", row_vaddr);
            uint32_t row_end = off + 16u;
            if (row_end > s->size) row_end = s->size;
            for (uint32_t j = off; j < off + 16u; j++) {
                if (j < row_end) fprintf(out, "%02x ", data[j]);
                else             fprintf(out, "   ");
            }
            fprintf(out, " |");
            for (uint32_t j = off; j < row_end; j++) {
                uint8_t c = data[j];
                fputc((c >= 0x20 && c < 0x7f) ? (int)c : '.', out);
            }
            fprintf(out, "|\n");
            for (uint32_t j = off; j + 4u <= row_end; j += 4u) {
                if (((row_vaddr + (j - off)) & 3u) != 0) continue;
                uint32_t word = ((uint32_t)data[j] << 24) |
                                ((uint32_t)data[j + 1] << 16) |
                                ((uint32_t)data[j + 2] <<  8) |
                                 (uint32_t)data[j + 3];
                const SymEntry *e = sym_covering(syms, word);
                if (!e) continue;
                uint32_t ptr_ofs = word - e->vaddr;
                if (ptr_ofs == 0)
                    fprintf(out, "               ; +0x%x -> &%s\n",
                            j - off, e->name);
                else
                    fprintf(out, "               ; +0x%x -> &%s+0x%x\n",
                            j - off, e->name, ptr_ofs);
            }
        }
    }
}

void dump_strings(const DolImage *img, const SymTable *syms, FILE *out) {
    const unsigned minlen = 4;
    fprintf(out, "\nstring scan (min length %u):\n", minlen);
    unsigned total = 0;
    for (size_t i = 0; i < img->section_count; i++) {
        const DolSection *s = &img->sections[i];
        if (s->kind != DOL_SECTION_DATA) continue;
        const uint8_t *data = img->bytes + s->file_offset;
        uint32_t run_start_off = 0;
        unsigned run_len = 0;
        int in_run = 0;
        for (uint32_t off = 0; off < s->size; off++) {
            uint8_t c = data[off];
            int printable = (c >= 0x20 && c < 0x7f) || c == '\t';
            if (printable) {
                if (!in_run) { run_start_off = off; run_len = 0; in_run = 1; }
                run_len++;
            } else if (c == 0x00 && in_run && run_len >= minlen) {
                uint32_t run_vaddr = s->virtual_addr + run_start_off;
                const char *sym = sym_lookup(syms, run_vaddr);
                fprintf(out, "  0x%08x", run_vaddr);
                if (sym) fprintf(out, " <%s>", sym);
                fprintf(out, ": \"");
                for (unsigned k = 0; k < run_len; k++) {
                    uint8_t ch = data[run_start_off + k];
                    if (ch == '\\' || ch == '"') fputc('\\', out);
                    fputc((int)ch, out);
                }
                fprintf(out, "\"\n");
                total++;
                in_run = 0;
            } else {
                in_run = 0;
            }
        }
    }
    fprintf(out, "\n%u string(s) found.\n", total);
}
