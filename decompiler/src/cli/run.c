#include "cli/run.h"
#include "cli/analysis.h"
#include "cli/bulk.h"
#include "cli/cfg.h"
#include "cli/ssa.h"
#include "cli/types.h"
#include "cli/classes.h"
#include "cli/demangle.h"
#include "cli/externs.h"
#include "cli/emit_cal.h"
#include "cli/extract.h"
#include "dol.h"
#include "run_internal.h"

#include <stdio.h>
#include <stdlib.h>

int disassemble_dol(const uint8_t *bytes, size_t n,
                    const RunOptions *o,
                    const SymTable *syms, FILE *out) {
    DolImage img;
    DolStatus st = dol_parse(bytes, n, &img);
    if (st != DOL_OK) {
        fprintf(stderr, "error: dol: %s\n", dol_status_str(st));
        return 1;
    }
    fprintf(out, "dol entrypoint: 0x%08x\n", img.entrypoint);
    fprintf(out, "dol sections:   %zu\n", img.section_count);
    for (size_t i = 0; i < img.section_count; i++) {
        const DolSection *s = &img.sections[i];
        fprintf(out, "  [%zu] %s%u  file=0x%08x  vaddr=0x%08x  size=0x%08x\n",
               i, s->kind == DOL_SECTION_TEXT ? "text" : "data",
               s->index, s->file_offset, s->virtual_addr, s->size);
    }
    fprintf(out, "bss:            0x%08x  size=0x%08x\n",
            img.bss_addr, img.bss_size);

    if (o->extract_dir && extract_sections(&img, o->extract_dir) != 0)
        return 1;

    if (o->call_graph_path) {
        FILE *cg = fopen(o->call_graph_path, "w");
        if (!cg) { perror(o->call_graph_path); return 1; }
        int rc = emit_call_graph(&img, syms, cg);
        fclose(cg);
        if (rc != 0) return rc;
        fprintf(stderr, "call graph written to: %s\n", o->call_graph_path);
    }

    if (o->dump_data_flag) dump_data(&img, syms, out);
    if (o->strings_flag)   dump_strings(&img, syms, out);
    if (o->xrefs_name)     return emit_xrefs(&img, syms, o->xrefs_name, out);
    if (o->section_name)   return disassemble_section(&img, o->section_name,
                                                      syms, out);
    if (o->all_functions_flag) return emit_all_functions(&img, syms, out);
    if (o->stats_flag)         return emit_stats(&img, syms, out);
    if (o->cfg_name) {
        const SymEntry *e = sym_find_by_name(syms, o->cfg_name);
        if (!e) {
            fprintf(stderr, "error: symbol not found: %s\n", o->cfg_name);
            return 1;
        }
        if (e->size == 0) {
            fprintf(stderr, "error: symbol %s has unknown size\n",
                    o->cfg_name);
            return 1;
        }
        Cfg cfg;
        if (cfg_build(&img, e->vaddr, e->vaddr + e->size, &cfg) != 0)
            return 1;
        fprintf(out, "function: %s  0x%08x..0x%08x\n",
                e->name, e->vaddr, e->vaddr + e->size);
        cfg_print(&cfg, out);
        cfg_free(&cfg);
        return 0;
    }
    if (o->ssa_name) {
        const SymEntry *e = sym_find_by_name(syms, o->ssa_name);
        if (!e) {
            fprintf(stderr, "error: symbol not found: %s\n", o->ssa_name);
            return 1;
        }
        if (e->size == 0) {
            fprintf(stderr, "error: symbol %s has unknown size\n",
                    o->ssa_name);
            return 1;
        }
        Cfg cfg;
        if (cfg_build(&img, e->vaddr, e->vaddr + e->size, &cfg) != 0)
            return 1;
        Ssa ssa;
        if (ssa_build(&img, &cfg, &ssa) != 0) { cfg_free(&cfg); return 1; }
        fprintf(out, "function: %s  0x%08x..0x%08x\n",
                e->name, e->vaddr, e->vaddr + e->size);
        ssa_print(&ssa, out);
        ssa_free(&ssa);
        cfg_free(&cfg);
        return 0;
    }
    if (o->types_name) {
        const SymEntry *e = sym_find_by_name(syms, o->types_name);
        if (!e) {
            fprintf(stderr, "error: symbol not found: %s\n", o->types_name);
            return 1;
        }
        if (e->size == 0) {
            fprintf(stderr, "error: symbol %s has unknown size\n",
                    o->types_name);
            return 1;
        }
        Cfg cfg;
        if (cfg_build(&img, e->vaddr, e->vaddr + e->size, &cfg) != 0)
            return 1;
        Ssa ssa;
        if (ssa_build(&img, &cfg, &ssa) != 0) { cfg_free(&cfg); return 1; }
        Types ty;
        if (types_build(&ssa, syms, NULL, &ty) != 0) {
            ssa_free(&ssa); cfg_free(&cfg); return 1;
        }
        fprintf(out, "function: %s  0x%08x..0x%08x\n",
                e->name, e->vaddr, e->vaddr + e->size);
        types_print(&ty, out);
        types_free(&ty);
        ssa_free(&ssa);
        cfg_free(&cfg);
        return 0;
    }
    if (o->classes) {
        Classes cls;
        if (classes_build(&img, syms, &cls) != 0) {
            fprintf(stderr, "error: classes_build failed\n");
            return 1;
        }
        classes_print(&cls, out);
        classes_free(&cls);
        return 0;
    }
    if (o->demangle) {
        CwRenameTable *tab = cw_rename_new();
        fprintf(out, "# Demangled symbols\n");
        for (size_t i = 0; i < syms->count; i++) {
            const SymEntry *e = &syms->entries[i];
            CwDemangled d;
            cw_demangle(e->name, &d);
            char cal[320], unique[360];
            cw_to_calynda_binding(&d, cal, sizeof cal);
            cw_rename_unique(tab, cal[0] ? cal : e->name,
                             unique, sizeof unique);
            fprintf(out, "0x%08x  %s\n", e->vaddr, e->name);
            if (d.ok)
                fprintf(out, "    cpp:     %s\n", d.pretty);
            if (cal[0])
                fprintf(out, "    calynda: %s\n", unique);
        }
        cw_rename_free(tab);
        return 0;
    }
    if (o->externs_dir) {
        Externs ex;
        if (externs_build(&img, syms, &ex) != 0) {
            fprintf(stderr, "error: externs_build failed\n");
            return 1;
        }
        int files = externs_emit_dir(&ex, o->externs_dir);
        if (files < 0) {
            fprintf(stderr, "error: could not write to %s\n",
                    o->externs_dir);
            externs_free(&ex);
            return 1;
        }
        externs_print(&ex, out);
        fprintf(stderr, "externs: %zu symbols, %d .cal files under %s\n",
                ex.count, files, o->externs_dir);
        externs_free(&ex);
        return 0;
    }
    if (o->emit_cal_dir) {
        Externs ex;
        if (externs_build(&img, syms, &ex) != 0) {
            fprintf(stderr, "error: externs_build failed\n");
            return 1;
        }
        EmitCalOpts eo = {
            .img = &img, .syms = syms, .externs = &ex,
            .out_dir = o->emit_cal_dir,
            .line_limit = 0, .max_functions = 0,
        };
        EmitCalStats es = {0};
        int rc = emit_cal(&eo, &es);
        externs_free(&ex);
        if (rc != 0) {
            fprintf(stderr, "error: emit_cal failed (out=%s)\n",
                    o->emit_cal_dir);
            return 1;
        }
        fprintf(stderr,
                "emit-cal: %u modules, %u files, %u functions, %u externs, "
                "%u globals, %u split siblings (out=%s)\n",
                es.modules_emitted, es.files_written, es.functions_emitted,
                es.externs_emitted, es.globals_emitted, es.split_siblings,
                o->emit_cal_dir);
        return 0;
    }
    if (o->diff_path) {
        size_t bn = 0;
        uint8_t *bb = run_slurp_file(o->diff_path, &bn);
        if (!bb) return 1;
        DolImage other;
        DolStatus bs = dol_parse(bb, bn, &other);
        if (bs != DOL_OK) {
            fprintf(stderr, "error: diff dol: %s\n", dol_status_str(bs));
            free(bb);
            return 1;
        }
        int rc = emit_diff(&img, &other, syms, out);
        free(bb);
        return rc;
    }

    if (o->function_name) {
        const SymEntry *e = sym_find_by_name(syms, o->function_name);
        if (!e) {
            fprintf(stderr, "error: symbol not found: %s\n", o->function_name);
            return 1;
        }
        if (e->size == 0) {
            fprintf(stderr,
                    "error: symbol %s has unknown size in the symbol file\n",
                    o->function_name);
            return 1;
        }
        size_t remaining = 0;
        const uint8_t *fip = dol_vaddr_to_ptr(&img, e->vaddr, &remaining);
        if (!fip) {
            fprintf(stderr,
                    "error: symbol %s at 0x%08x is not mapped in this DOL\n",
                    o->function_name, e->vaddr);
            return 1;
        }
        unsigned count = e->size / 4u;
        if ((size_t)count * 4u > remaining) count = (unsigned)(remaining / 4u);
        fprintf(out, "\ndisassembly of %s (0x%08x, %u instructions):\n",
                o->function_name, e->vaddr, count);
        emit_disassembly_range(out, fip, e->vaddr, count, syms);
        return 0;
    }

    size_t remaining = 0;
    const uint8_t *ip = dol_vaddr_to_ptr(&img, img.entrypoint, &remaining);
    if (!ip) {
        fprintf(stderr, "error: entrypoint not mapped\n");
        return 1;
    }

    if (o->all_after_entry) {
        fprintf(out,
                "\ndisassembly (all text from entrypoint):\n");
        for (size_t i = 0; i < img.section_count; i++) {
            const DolSection *s = &img.sections[i];
            if (s->kind != DOL_SECTION_TEXT) continue;
            if (img.entrypoint >= s->virtual_addr + s->size) continue;
            uint32_t start_vaddr = img.entrypoint > s->virtual_addr
                ? img.entrypoint : s->virtual_addr;
            size_t sec_rem = 0;
            const uint8_t *sip = dol_vaddr_to_ptr(&img, start_vaddr, &sec_rem);
            if (!sip) continue;
            fprintf(out, "\ntext%u @ 0x%08x:\n", s->index, start_vaddr);
            emit_disassembly_range(out, sip, start_vaddr,
                                   (unsigned)(sec_rem / 4u), syms);
        }
        return 0;
    }

    unsigned max = (unsigned)(remaining / 4u);
    if (max > o->insns) max = o->insns;
    fprintf(out, "\ndisassembly (%u instructions at entrypoint):\n", max);
    emit_disassembly_range(out, ip, img.entrypoint, max, syms);
    return 0;
}

long run_file_size(FILE *fp) {
    if (fseek(fp, 0, SEEK_END) != 0) return -1;
    long n = ftell(fp);
    if (fseek(fp, 0, SEEK_SET) != 0) return -1;
    return n;
}

uint8_t *run_slurp_file(const char *path, size_t *n_out) {
    FILE *fp = fopen(path, "rb");
    if (!fp) { perror(path); return NULL; }
    long n = run_file_size(fp);
    if (n < 0) {
        fclose(fp); fprintf(stderr, "error: seek failed\n"); return NULL;
    }
    uint8_t *buf = (uint8_t *)malloc((size_t)n);
    if (!buf) {
        fclose(fp); fprintf(stderr, "error: out of memory\n"); return NULL;
    }
    if (fread(buf, 1, (size_t)n, fp) != (size_t)n) {
        free(buf); fclose(fp);
        fprintf(stderr, "error: read failed\n");
        return NULL;
    }
    fclose(fp);
    *n_out = (size_t)n;
    return buf;
}

int run_on_dol_file(const char *path, const RunOptions *opts,
                    const SymTable *syms, FILE *out) {
    size_t n = 0;
    uint8_t *buf = run_slurp_file(path, &n);
    if (!buf) return 1;
    int rc = disassemble_dol(buf, n, opts, syms, out);
    free(buf);
    return rc;
}
