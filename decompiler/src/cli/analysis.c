#include "cli/analysis.h"
#include "ppc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int is_direct_branch(PpcOp op) {
    return op == PPC_OP_B  || op == PPC_OP_BA ||
           op == PPC_OP_BL || op == PPC_OP_BLA ||
           op == PPC_OP_BC;
}

/* Append "  <name>" or "  <name+0xN>" to `buf` when `in` is a direct
 * branch whose target resolves via `syms`. */
static void annotate_branch_target(char *buf, size_t bufsz,
                                   const PpcInsn *in, const SymTable *syms) {
    if (!is_direct_branch(in->op)) return;
    if (!syms || !syms->count) return;
    const SymEntry *e = sym_covering(syms, in->target);
    if (!e) return;
    size_t len = strlen(buf);
    if (len >= bufsz) return;
    uint32_t ofs = in->target - e->vaddr;
    if (ofs == 0)
        snprintf(buf + len, bufsz - len, "  <%s>", e->name);
    else
        snprintf(buf + len, bufsz - len, "  <%s+0x%x>", e->name, ofs);
}

void emit_disassembly_range(FILE *out, const uint8_t *ip,
                            uint32_t start_vaddr, unsigned insn_count,
                            const SymTable *syms) {
    for (unsigned i = 0; i < insn_count; i++) {
        uint32_t vaddr = start_vaddr + i * 4u;
        uint32_t word  = ((uint32_t)ip[0] << 24) | ((uint32_t)ip[1] << 16) |
                         ((uint32_t)ip[2] <<  8) |  (uint32_t)ip[3];
        ip += 4;
        const char *sym = sym_lookup(syms, vaddr);
        if (sym) fprintf(out, "\n<%s>:\n", sym);
        PpcInsn in;
        ppc_decode(word, vaddr, &in);
        char buf[160];
        ppc_format(&in, buf, sizeof(buf));
        annotate_branch_target(buf, sizeof(buf), &in, syms);
        fprintf(out, "  0x%08x: %08x  %s\n", vaddr, word, buf);
    }
}

int disassemble_section(const DolImage *img, const char *name,
                        const SymTable *syms, FILE *out) {
    for (size_t i = 0; i < img->section_count; i++) {
        const DolSection *s = &img->sections[i];
        char sname[16];
        snprintf(sname, sizeof(sname), "%s%u",
                 s->kind == DOL_SECTION_TEXT ? "text" : "data", s->index);
        if (strcmp(sname, name) != 0) continue;
        if (s->kind != DOL_SECTION_TEXT) {
            fprintf(stderr,
                    "error: --section only supports text sections (got %s)\n",
                    sname);
            return 1;
        }
        const uint8_t *ip = img->bytes + s->file_offset;
        unsigned count = s->size / 4u;
        fprintf(out, "\ndisassembly of %s (0x%08x, %u instructions):\n",
                sname, s->virtual_addr, count);
        emit_disassembly_range(out, ip, s->virtual_addr, count, syms);
        return 0;
    }
    fprintf(stderr, "error: section not found: %s\n", name);
    return 1;
}

int emit_call_graph(const DolImage *img, const SymTable *syms, FILE *out) {
    if (!syms || !syms->count) {
        fprintf(stderr, "error: --call-graph requires --symbols\n");
        return 1;
    }
    unsigned edges = 0;
    for (size_t i = 0; i < img->section_count; i++) {
        const DolSection *s = &img->sections[i];
        if (s->kind != DOL_SECTION_TEXT) continue;
        const uint8_t *data = img->bytes + s->file_offset;
        for (uint32_t off = 0; off < s->size; off += 4u) {
            const uint8_t *ip = data + off;
            uint32_t word = ((uint32_t)ip[0] << 24) | ((uint32_t)ip[1] << 16) |
                            ((uint32_t)ip[2] <<  8) |  (uint32_t)ip[3];
            uint32_t vaddr = s->virtual_addr + off;
            PpcInsn in;
            ppc_decode(word, vaddr, &in);
            if (in.op != PPC_OP_BL && in.op != PPC_OP_BLA) continue;
            const SymEntry *caller = sym_covering(syms, vaddr);
            const char *callee_name = sym_lookup(syms, in.target);
            char caller_buf[32], callee_buf[32];
            const char *caller_s, *callee_s;
            if (caller) caller_s = caller->name;
            else {
                snprintf(caller_buf, sizeof(caller_buf), "0x%08x", vaddr);
                caller_s = caller_buf;
            }
            if (callee_name) callee_s = callee_name;
            else {
                snprintf(callee_buf, sizeof(callee_buf), "0x%08x", in.target);
                callee_s = callee_buf;
            }
            fprintf(out, "%s -> %s\n", caller_s, callee_s);
            edges++;
        }
    }
    fprintf(stderr, "call graph: %u edges\n", edges);
    return 0;
}

int emit_xrefs(const DolImage *img, const SymTable *syms,
               const char *target_name, FILE *out) {
    if (!syms || !syms->count) {
        fprintf(stderr, "error: --xrefs requires --symbols\n");
        return 1;
    }
    const SymEntry *target = sym_find_by_name(syms, target_name);
    if (!target) {
        fprintf(stderr, "error: symbol not found: %s\n", target_name);
        return 1;
    }
    uint32_t tstart = target->vaddr;
    uint32_t tend   = target->vaddr + (target->size ? target->size : 4u);
    fprintf(out, "\ncross-references to %s (0x%08x..0x%08x):\n",
            target_name, tstart, tend);
    unsigned xrefs = 0;
    for (size_t i = 0; i < img->section_count; i++) {
        const DolSection *s = &img->sections[i];
        if (s->kind != DOL_SECTION_TEXT) continue;
        const uint8_t *data = img->bytes + s->file_offset;
        for (uint32_t off = 0; off < s->size; off += 4u) {
            const uint8_t *ip = data + off;
            uint32_t word = ((uint32_t)ip[0] << 24) | ((uint32_t)ip[1] << 16) |
                            ((uint32_t)ip[2] <<  8) |  (uint32_t)ip[3];
            uint32_t vaddr = s->virtual_addr + off;
            PpcInsn in;
            ppc_decode(word, vaddr, &in);
            if (!is_direct_branch(in.op)) continue;
            if (in.target < tstart || in.target >= tend) continue;
            const char *kind =
                (in.op == PPC_OP_BL || in.op == PPC_OP_BLA) ? "bl" : "b";
            const SymEntry *caller = sym_covering(syms, vaddr);
            if (caller) {
                uint32_t ofs = vaddr - caller->vaddr;
                fprintf(out, "  0x%08x  %s  from %s+0x%x\n",
                        vaddr, kind, caller->name, ofs);
            } else {
                fprintf(out, "  0x%08x  %s  from 0x%08x\n",
                        vaddr, kind, vaddr);
            }
            xrefs++;
        }
    }
    fprintf(out, "\n%u xref(s) found.\n", xrefs);
    return 0;
}
