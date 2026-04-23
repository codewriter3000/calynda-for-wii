/*
 * Tests for the RVL SDK extern binding generator (requirement 6).
 */
#include "cli/externs.h"
#include "cli/symbols.h"
#include "dol.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void check_classify(const char *name, const char *expect) {
    const char *got = externs_classify(name);
    if (strcmp(got, expect) != 0)
        fprintf(stderr, "classify(%s): got=%s want=%s\n", name, got, expect);
    assert(strcmp(got, expect) == 0);
}

static void check_cpp_type(const char *cpp, const char *expect) {
    char out[128];
    externs_cpp_type_to_calynda(cpp, out, sizeof out);
    if (strcmp(out, expect) != 0)
        fprintf(stderr, "type(%s): got=%s want=%s\n", cpp, out, expect);
    assert(strcmp(out, expect) == 0);
}

int main(void) {
    /* 6.2 — SDK module classification. */
    check_classify("OSReport",                 "os");
    check_classify("GXSetMiscAttrib",          "gx");
    check_classify("PADRead",                  "pad");
    check_classify("WPADSetMotorSpeed",        "wpad");
    check_classify("AXRegisterVoice",          "ax");
    check_classify("DVDOpen",                  "dvd");
    check_classify("VIFlush",                  "vi");
    check_classify("memcpy",                   "msl");
    check_classify("__OSReboot",               "os");
    check_classify("nw4r::math::VEC3",         "nw4r");
    check_classify("EGG::Heap::alloc",         "egg");
    check_classify("MySceneClass",             "");
    printf("ok - classify\n");

    /* 6.3 / 6.4 — type string conversion. */
    check_cpp_type("int",                  "i32");
    check_cpp_type("unsigned int",         "u32");
    check_cpp_type("long",                 "i32");
    check_cpp_type("unsigned long long",   "u64");
    check_cpp_type("float",                "f32");
    check_cpp_type("void",                 "()");
    check_cpp_type("char*",                "*i8");
    check_cpp_type("const char*",          "*i8");
    check_cpp_type("DVDFileInfo*",         "*DVDFileInfo");
    check_cpp_type("const DVDFileInfo*",   "*DVDFileInfo");
    check_cpp_type("unsigned char**",      "**u8");
    check_cpp_type("nw4r::math::VEC3",     "nw4r.math.VEC3");
    check_cpp_type("nw4r::math::VEC3&",    "*nw4r.math.VEC3");
    check_cpp_type("void(*)(int, float)",  "fn(i32, f32) -> ()");
    check_cpp_type("...",                  "...");
    printf("ok - type conversion\n");

    /* 6.1 — build inventory against a synthesised DOL + symtable. */
    uint8_t blob[512] = { 0 };
    #define W32(o,v) do { blob[(o)]=((v)>>24)&0xff; blob[(o)+1]=((v)>>16)&0xff; \
                          blob[(o)+2]=((v)>>8)&0xff; blob[(o)+3]=(v)&0xff; } while(0)
    /* One text section: file=0x100, vaddr=0x80003000, size=0x10.        */
    W32(0x00, 0x00000100);  /* textOffs[0] */
    W32(0x48, 0x80003000);  /* textLoad[0] */
    W32(0x90, 0x00000010);  /* textSize[0] */
    W32(0xE0, 0x80003000);  /* entry */
    /* Instructions at text: bl 0x80003000+X, bl 0x81000000 (unmapped). */
    /* bl to offset +4 (self+4 covered by text) and bl to 0x81000000.    */
    /* bl = 0x48000001 | (LI<<2). Offset from 0x80003000 to 0x80003008
     * is +8 -> encoded as 0x48000009 (LI=2, LK=1).                      */
    W32(0x100, 0x48000009);  /* bl +8 (internal)    */
    /* bl to 0x81000000 from 0x80003004: displacement = 0xFD000000 which
     * does not fit in 26-bit signed (±32MB), so use bla absolute instead.
     * bla = 0x48000003 with AA=1, LK=1, LI = target>>2 but signed 24-bit.
     * Target 0x81000000 is out of range for bla too — use a target
     * within ±32MB.  Easiest: pick 0x80800000 (relative +0x7FCFFC from
     * 0x80003004).  Displacement = 0x007FCFFC which fits in 26 bits.   */
    {
        uint32_t pc = 0x80003004;
        uint32_t tgt = 0x80800000;
        uint32_t disp = (tgt - pc) & 0x03fffffc;
        W32(0x104, 0x48000001u | disp);
    }
    /* pad with nops */
    W32(0x108, 0x60000000);
    W32(0x10C, 0x4e800020);  /* blr */

    DolImage img;
    assert(dol_parse(blob, sizeof blob, &img) == DOL_OK);

    SymTable syms; sym_table_init(&syms);
    /* Register a few SDK symbols covering the `in-DOL but SDK-named`
     * case (6.1a) plus one unrelated game symbol. */
    #define ADD(v, n) do {                                               \
        SymEntry e = { 0 };                                              \
        e.vaddr = (v); e.size = 0x10;                                    \
        snprintf(e.name, sizeof e.name, "%s", (n));                      \
        if (syms.count == syms.cap) {                                    \
            syms.cap = syms.cap ? syms.cap * 2 : 8;                      \
            syms.entries = realloc(syms.entries,                         \
                                   syms.cap * sizeof(SymEntry));        \
        }                                                                \
        syms.entries[syms.count++] = e;                                  \
    } while (0)

    ADD(0x80003000, "OSReport");
    ADD(0x80003010, "GXSetMiscAttrib");
    ADD(0x80003020, "memcpy");
    ADD(0x80003030, "MyGameFn");
    ADD(0x80003040, "adjust__Q23EGG7ExpHeapFv");

    Externs ex;
    assert(externs_build(&img, &syms, &ex) == 0);
    assert(ex.count >= 5);

    int saw_os = 0, saw_gx = 0, saw_msl = 0, saw_egg = 0, saw_unmapped = 0;
    int saw_game = 0;
    for (size_t i = 0; i < ex.count; i++) {
        const ExternSymbol *s = &ex.entries[i];
        if (!strcmp(s->module, "os"))  saw_os = 1;
        if (!strcmp(s->module, "gx"))  saw_gx = 1;
        if (!strcmp(s->module, "msl")) saw_msl = 1;
        if (!strcmp(s->module, "egg")) saw_egg = 1;
        if (s->is_unmapped) saw_unmapped = 1;
        if (!strcmp(s->raw_name, "MyGameFn")) saw_game = 1;
    }
    assert(saw_os && saw_gx && saw_msl && saw_egg);
    assert(saw_unmapped);
    assert(!saw_game);
    printf("ok - inventory with SDK + unmapped detection\n");

    /* 6.5 — emit .cal files. */
    const char *tmp = "/tmp/cal_externs_test";
    /* Clear previous. */
    {
        char cmd[256];
        snprintf(cmd, sizeof cmd, "rm -rf %s", tmp);
        int rc = system(cmd); (void)rc;
    }
    int files = externs_emit_dir(&ex, tmp);
    assert(files >= 4);

    /* Each produced file should contain "extern " and "package rvl.". */
    FILE *f = fopen("/tmp/cal_externs_test/rvl_os.cal", "r");
    assert(f);
    char line[512];
    int saw_extern = 0, saw_pkg = 0;
    while (fgets(line, sizeof line, f)) {
        if (strstr(line, "extern ")) saw_extern = 1;
        if (strstr(line, "package rvl.os")) saw_pkg = 1;
    }
    fclose(f);
    assert(saw_extern && saw_pkg);
    printf("ok - emit rvl_os.cal\n");

    externs_free(&ex);
    sym_table_free(&syms);
    printf("-- all externs tests passed --\n");
    return 0;
}
