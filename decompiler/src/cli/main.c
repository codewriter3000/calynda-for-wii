/*
 * decompile: probe a DOL or Wii/GC ISO, report its format, and run
 * selected analysis passes over the embedded DOL image.
 *
 * For Wii ISOs, the tool lists partitions. Supply a Wii common key via
 * --common-key <file> or --key-hex <32 hex chars> to decrypt and
 * extract main.dol. No Nintendo keys are bundled with this tool.
 */
#include "cli/run.h"
#include "cli/symbols.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void print_usage(FILE *stream, const char *prog) {
    fprintf(stream,
        "usage: %s <file.dol|file.iso> [options]\n"
        "  --insns N              disassemble first N instructions (default 16)\n"
        "  --all-after-entry      disassemble all text from entrypoint onward\n"
        "  --function <name>      disassemble just the named symbol (needs --symbols)\n"
        "  --section <name>       disassemble an entire text section (e.g. text0)\n"
        "  --disasm-all-functions emit every sized symbol as a labelled block\n"
        "  --stats                print text coverage / decoded / gaps summary\n"
        "  --diff <other.dol>     byte-compare every sized symbol vs other DOL\n"
        "  --cfg <name>           build and print the CFG of the named symbol\n"
        "  --ssa <name>           build SSA def-use/phi/expr info for the symbol\n"
        "  --types <name>         run type inference on the symbol\n"
        "  --classes              reconstruct vtables and classes\n"
        "  --demangle             print C++/Calynda names for every symbol\n"
        "  --externs <dir>        emit rvl_<module>.cal extern bindings\n"
        "  --emit-cal <dir>       emit reconstructed Calynda source tree under <dir>\n"
        "  --xrefs <name>         list call sites branching into the named symbol\n"
        "  --strings              scan data sections for NUL-terminated strings\n"
        "  --dump-data            hex+ASCII dump of every data section\n"
        "  --call-graph <file>    write caller -> callee edges to <file>\n"
        "  --symbols <file>       annotate addresses using an ogws symbols.txt\n"
        "  --extract-sections <dir>  dump every DOL section as a raw binary file\n"
        "  --common-key <path>    16-byte binary Wii common-key file\n"
        "  --key-hex <hex>        same key as 32 hex chars pasted directly\n"
        "  --partition N          which Wii partition to use (default: first type-0)\n"
        "  --output <file>        write disassembly to <file> instead of stdout\n",
        prog);
}

static int usage(const char *prog)    { print_usage(stderr, prog); return 2; }
static int usage_ok(const char *prog) { print_usage(stdout, prog); return 0; }

static int has_suffix_ci(const char *s, const char *suffix) {
    size_t ls = strlen(s), lt = strlen(suffix);
    if (lt > ls) return 0;
    for (size_t i = 0; i < lt; i++) {
        char a = s[ls - lt + i]; if (a >= 'A' && a <= 'Z') a = a - 'A' + 'a';
        char b = suffix[i];      if (b >= 'A' && b <= 'Z') b = b - 'A' + 'a';
        if (a != b) return 0;
    }
    return 1;
}

typedef struct {
    const char  *path;
    const char  *key_path;
    const char  *key_hex;
    const char  *out_path;
    const char  *symbols_path;
    int          partition_arg;
    RunOptions   opts;
} Args;

static int parse_args(int argc, char **argv, Args *a) {
    memset(a, 0, sizeof(*a));
    a->path = argv[1];
    a->partition_arg = -1;
    a->opts.insns = 16;

    for (int i = 2; i < argc; i++) {
        const char *k = argv[i];
        int has_next = (i + 1) < argc;
        if      (!strcmp(k, "--insns") && has_next)
            a->opts.insns = (unsigned)strtoul(argv[++i], NULL, 0);
        else if (!strcmp(k, "--all-after-entry")) a->opts.all_after_entry = 1;
        else if (!strcmp(k, "--symbols") && has_next)
            a->symbols_path = argv[++i];
        else if (!strcmp(k, "--extract-sections") && has_next)
            a->opts.extract_dir = argv[++i];
        else if (!strcmp(k, "--function") && has_next)
            a->opts.function_name = argv[++i];
        else if (!strcmp(k, "--dump-data")) a->opts.dump_data_flag = 1;
        else if (!strcmp(k, "--strings"))   a->opts.strings_flag = 1;
        else if (!strcmp(k, "--xrefs") && has_next)
            a->opts.xrefs_name = argv[++i];
        else if (!strcmp(k, "--section") && has_next)
            a->opts.section_name = argv[++i];
        else if (!strcmp(k, "--disasm-all-functions"))
            a->opts.all_functions_flag = 1;
        else if (!strcmp(k, "--stats"))
            a->opts.stats_flag = 1;
        else if (!strcmp(k, "--diff") && has_next)
            a->opts.diff_path = argv[++i];
        else if (!strcmp(k, "--cfg") && has_next)
            a->opts.cfg_name = argv[++i];
        else if (!strcmp(k, "--ssa") && has_next)
            a->opts.ssa_name = argv[++i];
        else if (!strcmp(k, "--types") && has_next)
            a->opts.types_name = argv[++i];
        else if (!strcmp(k, "--classes"))
            a->opts.classes = 1;
        else if (!strcmp(k, "--demangle"))
            a->opts.demangle = 1;
        else if (!strcmp(k, "--externs") && has_next)
            a->opts.externs_dir = argv[++i];
        else if (!strcmp(k, "--emit-cal") && has_next)
            a->opts.emit_cal_dir = argv[++i];
        else if (!strcmp(k, "--call-graph") && has_next)
            a->opts.call_graph_path = argv[++i];
        else if (!strcmp(k, "--common-key") && has_next)
            a->key_path = argv[++i];
        else if (!strcmp(k, "--key-hex") && has_next)
            a->key_hex = argv[++i];
        else if (!strcmp(k, "--partition") && has_next)
            a->partition_arg = (int)strtol(argv[++i], NULL, 0);
        else if (!strcmp(k, "--output") && has_next)
            a->out_path = argv[++i];
        else if (!strcmp(k, "-h") || !strcmp(k, "--help"))
            return -1;  /* print help and exit 0 */
        else
            return 0;   /* bad arg */
    }
    if (a->key_path && a->key_hex) {
        fprintf(stderr, "error: --common-key and --key-hex are mutually exclusive\n");
        return 0;
    }
    return 1;
}

int main(int argc, char **argv) {
    if (argc < 2) return usage(argv[0]);
    if (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help"))
        return usage_ok(argv[0]);

    Args a;
    int pr = parse_args(argc, argv, &a);
    if (pr < 0) return usage_ok(argv[0]);
    if (pr == 0) return usage(argv[0]);

    FILE *out = stdout;
    if (a.out_path) {
        out = fopen(a.out_path, "w");
        if (!out) { perror(a.out_path); return 1; }
        fprintf(stderr, "writing disassembly to: %s\n", a.out_path);
    }

    SymTable syms;
    sym_table_init(&syms);
    if (a.symbols_path) {
        if (!sym_table_load(&syms, a.symbols_path)) {
            if (out != stdout) fclose(out);
            sym_table_free(&syms);
            return 1;
        }
        fprintf(stderr, "loaded %zu symbols from: %s\n",
                syms.count, a.symbols_path);
    }

    int rc;
    if (has_suffix_ci(a.path, ".iso"))
        rc = run_on_iso_file(a.path, &a.opts, a.key_path, a.key_hex,
                             a.partition_arg, &syms, out);
    else
        rc = run_on_dol_file(a.path, &a.opts, &syms, out);

    sym_table_free(&syms);
    if (out != stdout) fclose(out);

    if (rc == 0) {
        if (a.out_path)
            fprintf(stderr, "done. disassembly written to: %s\n", a.out_path);
        else
            fprintf(stderr, "done.\n");
    } else {
        fprintf(stderr, "failed (exit code %d)\n", rc);
    }
    return rc;
}
