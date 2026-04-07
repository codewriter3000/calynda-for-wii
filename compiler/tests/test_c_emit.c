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

#define ASSERT_CONTAINS(needle, haystack, msg) do {                         \
    tests_run++;                                                            \
    if ((haystack) != NULL && strstr((haystack), (needle)) != NULL) {       \
        tests_passed++;                                                     \
    } else {                                                                \
        tests_failed++;                                                     \
        fprintf(stderr, "  FAIL [%s:%d] %s: missing \"%s\" in output\n",  \
                __FILE__, __LINE__, (msg), (needle));                       \
    }                                                                       \
} while (0)

#define RUN_TEST(fn) do {                                                   \
    printf("  %s ...\n", #fn);                                            \
    fn();                                                                   \
} while (0)

/* Build a HirProgram from Calynda source text.  Returns true on success. */
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

/* Emit a HIR program to a memory buffer. Returns a malloc'd string. */
static char *emit_to_string(const HirProgram *hir) {
    FILE  *stream = tmpfile();
    char  *buffer = NULL;
    long   size;
    size_t read_size;
    bool   ok;

    if (!stream) {
        return NULL;
    }
    ok = c_emit_program(stream, hir);
    if (!ok) {
        fclose(stream);
        return NULL;
    }
    fflush(stream);
    fseek(stream, 0, SEEK_END);
    size = ftell(stream);
    fseek(stream, 0, SEEK_SET);
    if (size < 0) {
        fclose(stream);
        return NULL;
    }
    buffer = malloc((size_t)size + 1);
    if (!buffer) {
        fclose(stream);
        return NULL;
    }
    read_size = fread(buffer, 1, (size_t)size, stream);
    fclose(stream);
    buffer[read_size] = '\0';
    return buffer;
}

/* ------------------------------------------------------------------ */

static void test_c_emit_simple_start(void) {
    static const char source[] = "start(string[] args) -> 7;\n";
    Parser      parser;
    AstProgram  ast;
    SymbolTable symbols;
    TypeChecker checker;
    HirProgram  hir;
    char       *output;

    REQUIRE_TRUE(build_hir(source, &hir, &parser, &ast, &symbols, &checker),
                 "build HIR for simple start program");

    output = emit_to_string(&hir);
    REQUIRE_TRUE(output != NULL, "c_emit_program succeeds");

    ASSERT_CONTAINS("#include", output, "output contains #include");
    ASSERT_CONTAINS("int main", output, "output contains int main");
    ASSERT_CONTAINS("calynda_runtime.h", output, "output references calynda_runtime.h");
    ASSERT_CONTAINS("__calynda_start", output, "output defines __calynda_start");
    ASSERT_CONTAINS("calynda_rt_start_process", output, "output calls start_process");

    free(output);
    hir_program_free(&hir);
    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&ast);
    parser_free(&parser);
}

static void test_c_emit_binding_with_lambda(void) {
    static const char source[] =
        "int32 add = (int32 left, int32 right) -> left + right;\n"
        "start(string[] args) -> add((1), 2);\n";
    Parser      parser;
    AstProgram  ast;
    SymbolTable symbols;
    TypeChecker checker;
    HirProgram  hir;
    char       *output;

    REQUIRE_TRUE(build_hir(source, &hir, &parser, &ast, &symbols, &checker),
                 "build HIR for program with lambda");

    output = emit_to_string(&hir);
    REQUIRE_TRUE(output != NULL, "c_emit_program succeeds with lambda");

    ASSERT_CONTAINS("#include", output, "output contains #include");
    ASSERT_CONTAINS("int main", output, "output contains int main");
    ASSERT_CONTAINS("__calynda_lambda_", output, "output contains lambda function");
    ASSERT_CONTAINS("__calynda_rt_closure_new", output, "output creates closure");
    ASSERT_CONTAINS("__calynda_rt_call_callable", output, "output calls callable");

    free(output);
    hir_program_free(&hir);
    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&ast);
    parser_free(&parser);
}

static void test_c_emit_returns_zero_without_start(void) {
    /* Emit on an empty (just-initialised) HirProgram to exercise the fallback
       start function emitter.  We skip HIR building entirely. */
    HirProgram hir;
    char      *output;

    hir_program_init(&hir);

    output = emit_to_string(&hir);
    REQUIRE_TRUE(output != NULL, "c_emit_program succeeds on empty HIR");

    ASSERT_CONTAINS("int main", output, "output contains int main even with empty HIR");
    ASSERT_CONTAINS("#include", output, "output contains #include");
    ASSERT_CONTAINS("__calynda_start", output, "output contains fallback start");

    free(output);
    hir_program_free(&hir);
}

static void test_c_emit_boot_entry_point(void) {
    const char *source =
        "boot() -> {\n"
        "    return;\n"
        "};\n";
    HirProgram hir;
    Parser     parser;
    AstProgram ast;
    SymbolTable symbols;
    TypeChecker checker;
    char *output;

    REQUIRE_TRUE(build_hir(source, &hir, &parser, &ast, &symbols, &checker),
                 "build HIR for boot program");

    output = emit_to_string(&hir);
    REQUIRE_TRUE(output != NULL, "c_emit_program succeeds for boot program");

    ASSERT_CONTAINS("__calynda_boot", output, "output contains boot function");
    ASSERT_CONTAINS("static void __calynda_boot(void)", output, "boot function is void");
    ASSERT_CONTAINS("(void)argc; (void)argv;", output, "boot main ignores args");
    ASSERT_CONTAINS("__calynda_boot();", output, "main calls boot");

    free(output);
    hir_program_free(&hir);
    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&ast);
    parser_free(&parser);
}

static void test_c_emit_extern_declaration(void) {
    const char *source =
        "extern int32 printf = c(string fmt, ...);\n"
        "start(string[] args) -> {\n"
        "    return 0;\n"
        "};\n";
    HirProgram hir;
    Parser     parser;
    AstProgram ast;
    SymbolTable symbols;
    TypeChecker checker;
    char *output;

    REQUIRE_TRUE(build_hir(source, &hir, &parser, &ast, &symbols, &checker),
                 "build HIR for extern program");

    output = emit_to_string(&hir);
    REQUIRE_TRUE(output != NULL, "c_emit_program succeeds for extern program");

    ASSERT_CONTAINS("extern CalyndaRtWord printf", output,
                    "output contains extern printf forward declaration");

    free(output);
    hir_program_free(&hir);
    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&ast);
    parser_free(&parser);
}

/* ------------------------------------------------------------------ */

int main(void) {
    printf("Running c_emit tests...\n\n");

    RUN_TEST(test_c_emit_simple_start);
    RUN_TEST(test_c_emit_binding_with_lambda);
    RUN_TEST(test_c_emit_returns_zero_without_start);
    RUN_TEST(test_c_emit_boot_entry_point);
    RUN_TEST(test_c_emit_extern_declaration);

    printf("\n========================================\n");
    printf("  Total: %d  |  Passed: %d  |  Failed: %d\n",
           tests_run, tests_passed, tests_failed);
    printf("========================================\n");

    return tests_failed > 0 ? 1 : 0;
}
