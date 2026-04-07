/* test_c_compile.c — verify that generated C actually compiles with gcc -c */

#include "c_emit.h"
#include "hir.h"
#include "parser.h"
#include "symbol_table.h"
#include "type_checker.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int tests_run    = 0;
static int tests_passed = 0;
static int tests_failed = 0;

#define ASSERT_TRUE(condition, msg) do {                                    \
    tests_run++;                                                            \
    if (condition) {                                                        \
        tests_passed++;                                                     \
    } else {                                                                \
        tests_failed++;                                                     \
        fprintf(stderr, "  FAIL [%s:%d] %s\n",                            \
                __FILE__, __LINE__, (msg));                                 \
    }                                                                       \
} while (0)

#define REQUIRE_TRUE(condition, msg) do {                                   \
    tests_run++;                                                            \
    if (condition) {                                                        \
        tests_passed++;                                                     \
    } else {                                                                \
        tests_failed++;                                                     \
        fprintf(stderr, "  FAIL [%s:%d] %s\n",                            \
                __FILE__, __LINE__, (msg));                                 \
        return;                                                             \
    }                                                                       \
} while (0)

#define RUN_TEST(fn) do {                                                   \
    printf("  %s ...\n", #fn);                                            \
    fn();                                                                   \
} while (0)

/* ------------------------------------------------------------------ */
/* Helpers                                                              */
/* ------------------------------------------------------------------ */

static bool build_hir(const char *source,
                      HirProgram *hir,
                      Parser *parser,
                      AstProgram *ast,
                      SymbolTable *symbols,
                      TypeChecker *checker) {
    hir_program_init(hir);
    symbol_table_init(symbols);
    type_checker_init(checker);
    parser_init(parser, source);

    if (!parser_parse_program(parser, ast)) {
        return false;
    }
    if (!symbol_table_build(symbols, ast)) {
        return false;
    }
    if (!type_checker_check_program(checker, ast, symbols)) {
        return false;
    }
    return hir_build_program(hir, ast, symbols, checker);
}

/* Emit C to a temp file, then compile with gcc -c.
 * Returns 0 on gcc success, non-zero on failure. */
static int emit_and_compile(const HirProgram *hir,
                             const char *runtime_include_dir) {
    FILE *f;
    char c_file[256];
    char obj_file[256];
    char cmd[1024];
    int  rc;

    snprintf(c_file,   sizeof(c_file),   "/tmp/test_c_compile_%d.c",   (int)getpid());
    snprintf(obj_file, sizeof(obj_file),  "/tmp/test_c_compile_%d.o",   (int)getpid());

    f = fopen(c_file, "w");
    if (!f) {
        return -1;
    }

    if (!c_emit_program(f, hir)) {
        fclose(f);
        return -1;
    }
    fclose(f);

    snprintf(cmd, sizeof(cmd),
             "gcc -std=c11 -c \"%s\" -I\"%s\" -o \"%s\" 2>/dev/null",
             c_file, runtime_include_dir, obj_file);

    rc = system(cmd);

    remove(c_file);
    remove(obj_file);
    return rc;
}

/* ------------------------------------------------------------------ */
/* Tests                                                                */
/* ------------------------------------------------------------------ */

static void test_compile_minimal_start(void) {
    const char *source = "start(string[] args) -> 0;\n";
    HirProgram hir;
    Parser     parser;
    AstProgram ast;
    SymbolTable symbols;
    TypeChecker checker;

    REQUIRE_TRUE(build_hir(source, &hir, &parser, &ast, &symbols, &checker),
                 "build HIR for minimal start");

    ASSERT_TRUE(emit_and_compile(&hir, "src/c_runtime") == 0,
                "minimal start compiles with gcc -c");

    hir_program_free(&hir);
    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&ast);
    parser_free(&parser);
}

static void test_compile_boot_entry(void) {
    const char *source =
        "boot() -> {\n"
        "    return;\n"
        "};\n";
    HirProgram hir;
    Parser     parser;
    AstProgram ast;
    SymbolTable symbols;
    TypeChecker checker;

    REQUIRE_TRUE(build_hir(source, &hir, &parser, &ast, &symbols, &checker),
                 "build HIR for boot entry");

    ASSERT_TRUE(emit_and_compile(&hir, "src/c_runtime") == 0,
                "boot() program compiles with gcc -c");

    hir_program_free(&hir);
    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&ast);
    parser_free(&parser);
}

static void test_compile_extern_c_ffi(void) {
    const char *source =
        "extern int32 my_puts = c(string s);\n"
        "start(string[] args) -> {\n"
        "    return 0;\n"
        "};\n";
    HirProgram hir;
    Parser     parser;
    AstProgram ast;
    SymbolTable symbols;
    TypeChecker checker;

    REQUIRE_TRUE(build_hir(source, &hir, &parser, &ast, &symbols, &checker),
                 "build HIR for extern C FFI");

    ASSERT_TRUE(emit_and_compile(&hir, "src/c_runtime") == 0,
                "extern C FFI program compiles with gcc -c");

    hir_program_free(&hir);
    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&ast);
    parser_free(&parser);
}

static void test_compile_arithmetic(void) {
    const char *source =
        "int32 x = 10;\n"
        "int32 y = 20;\n"
        "start(string[] args) -> {\n"
        "    var result = x + y;\n"
        "    return result;\n"
        "};\n";
    HirProgram hir;
    Parser     parser;
    AstProgram ast;
    SymbolTable symbols;
    TypeChecker checker;

    REQUIRE_TRUE(build_hir(source, &hir, &parser, &ast, &symbols, &checker),
                 "build HIR for arithmetic program");

    ASSERT_TRUE(emit_and_compile(&hir, "src/c_runtime") == 0,
                "arithmetic program compiles with gcc -c");

    hir_program_free(&hir);
    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&ast);
    parser_free(&parser);
}

/* ------------------------------------------------------------------ */

int main(void) {
    printf("Running c_compile tests...\n\n");

    RUN_TEST(test_compile_minimal_start);
    RUN_TEST(test_compile_boot_entry);
    RUN_TEST(test_compile_extern_c_ffi);
    RUN_TEST(test_compile_arithmetic);

    printf("\n========================================\n");
    printf("  Total: %d  |  Passed: %d  |  Failed: %d\n",
           tests_run, tests_passed, tests_failed);
    printf("========================================\n");

    return tests_failed > 0 ? 1 : 0;
}
