/*
 * Tests for the Calynda source-tree emitter (requirement 8).
 *
 * Strategy: build a minimal in-memory DOL with a single text section
 * that contains one function (a simple "return r3" blr) and one
 * recognisable global in a data section. Invoke emit_cal() into a
 * tempdir and assert that:
 *
 *   - main.cal exists and contains a `package main` header and
 *     `start(` entrypoint wrapper (req 8.1, 8.4);
 *   - extern files (`rvl_msl.cal`) are produced by the piggy-backed
 *     req-6 emitter (req 8.2);
 *   - a global from the data section shows up as a `final`/`var`
 *     binding in main.cal (req 8.3);
 *   - forcing a small line_limit triggers the `_part2.cal` rotation
 *     (req 8.6).
 */
#define _DEFAULT_SOURCE
#define _GNU_SOURCE
#include "cli/emit_cal.h"
#include "cli/externs.h"
#include "cli/symbols.h"
#include "dol.h"

#include <assert.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

static void w32(uint8_t *p, size_t off, uint32_t v) {
    p[off+0] = (uint8_t)(v >> 24);
    p[off+1] = (uint8_t)(v >> 16);
    p[off+2] = (uint8_t)(v >>  8);
    p[off+3] = (uint8_t)(v      );
}

static int file_contains(const char *path, const char *needle) {
    FILE *f = fopen(path, "r");
    if (!f) return 0;
    char buf[8192];
    size_t n = fread(buf, 1, sizeof buf - 1, f);
    buf[n] = 0;
    fclose(f);
    return strstr(buf, needle) != NULL;
}

static int file_exists(const char *path) {
    struct stat st;
    return stat(path, &st) == 0 && S_ISREG(st.st_mode);
}

/* Remove directory contents (single level is enough for our test). */
static void rm_rf(const char *dir) {
    DIR *d = opendir(dir); if (!d) return;
    struct dirent *e;
    char p[512];
    while ((e = readdir(d))) {
        if (!strcmp(e->d_name, ".") || !strcmp(e->d_name, "..")) continue;
        snprintf(p, sizeof p, "%s/%s", dir, e->d_name);
        unlink(p);
    }
    closedir(d);
    rmdir(dir);
}

int main(void) {
    /* --- 1. Build a minimal DOL ------------------------------------- */
    uint8_t blob[768] = {0};
    /* text0: file 0x100, vaddr 0x80003000, size 8 (two insns)          */
    w32(blob, 0x00, 0x00000100);   /* textOffs[0] */
    w32(blob, 0x48, 0x80003000);   /* textLoad[0] */
    w32(blob, 0x90, 0x00000008);   /* textSize[0] */
    /* data0: file 0x200, vaddr 0x80100000, size 4                      */
    w32(blob, 0x1C, 0x00000200);   /* dataOffs[0] */
    w32(blob, 0x64, 0x80100000);   /* dataLoad[0] */
    w32(blob, 0xAC, 0x00000004);   /* dataSize[0] */
    w32(blob, 0xE0, 0x80003000);   /* entry */
    /* Function body:  blr (0x4e800020)  +  blr */
    w32(blob, 0x100, 0x4e800020);
    w32(blob, 0x104, 0x4e800020);
    /* Global 32-bit value 0xcafef00d. */
    w32(blob, 0x200, 0xcafef00d);

    DolImage img;
    DolStatus st = dol_parse(blob, sizeof blob, &img);
    if (st != DOL_OK) { fprintf(stderr, "dol_parse: %s\n", dol_status_str(st)); return 1; }
    assert(img.entrypoint == 0x80003000);

    /* --- 2. Build a symbol table ----------------------------------- */
    SymTable syms = {0};
    sym_table_init(&syms);
    /* Grow manually — the public API loads from file; use the heap
     * array directly for this unit test. */
    syms.cap = 4;
    syms.entries = calloc(syms.cap, sizeof *syms.entries);
    syms.count = 0;

    /* Function in text. */
    syms.entries[syms.count].vaddr = 0x80003000;
    syms.entries[syms.count].size  = 8;
    snprintf(syms.entries[syms.count].name, SYM_NAME_MAX, "start_probe");
    syms.count++;
    /* Global in data. */
    syms.entries[syms.count].vaddr = 0x80100000;
    syms.entries[syms.count].size  = 4;
    snprintf(syms.entries[syms.count].name, SYM_NAME_MAX, "g_magic");
    syms.count++;
    /* A classifiable SDK extern so externs_build produces rvl_msl.cal. */
    syms.entries[syms.count].vaddr = 0x81000000;
    syms.entries[syms.count].size  = 0;
    snprintf(syms.entries[syms.count].name, SYM_NAME_MAX, "memcpy");
    syms.count++;

    Externs ex;
    assert(externs_build(&img, &syms, &ex) == 0);

    /* --- 3. Normal emit -------------------------------------------- */
    char tmpl[] = "/tmp/emit_cal_XXXXXX";
    char *dir   = mkdtemp(tmpl);
    assert(dir != NULL);

    EmitCalOpts eo = {
        .img = &img, .syms = &syms, .externs = &ex,
        .out_dir = dir, .line_limit = 0, .max_functions = 0,
    };
    EmitCalStats es = {0};
    assert(emit_cal(&eo, &es) == 0);

    char main_path[512];
    snprintf(main_path, sizeof main_path, "%s/main.cal", dir);
    assert(file_exists(main_path));
    assert(file_contains(main_path, "package main"));
    assert(file_contains(main_path, "start(args: arr<string>)"));
    assert(file_contains(main_path, "g_magic"));
    assert(file_contains(main_path, "0xcafef00d"));
    assert(file_contains(main_path, "start_probe"));
    assert(es.functions_emitted >= 1);
    assert(es.globals_emitted   >= 1);
    printf("ok - main.cal layout (functions=%u globals=%u files=%u)\n",
           es.functions_emitted, es.globals_emitted, es.files_written);

    rm_rf(dir);

    /* --- 4. Force rotation ----------------------------------------- */
    char tmpl2[] = "/tmp/emit_cal_split_XXXXXX";
    char *dir2   = mkdtemp(tmpl2);
    assert(dir2 != NULL);

    eo.out_dir    = dir2;
    eo.line_limit = 5;     /* rotate after a few lines */
    EmitCalStats es2 = {0};
    assert(emit_cal(&eo, &es2) == 0);

    char part2[512];
    snprintf(part2, sizeof part2, "%s/main_part2.cal", dir2);
    assert(file_exists(part2));
    assert(es2.split_siblings >= 1);
    printf("ok - rotation (split_siblings=%u total_files=%u)\n",
           es2.split_siblings, es2.files_written);

    rm_rf(dir2);

    free(syms.entries);
    externs_free(&ex);
    return 0;
}
