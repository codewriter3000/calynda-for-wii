#include "calynda_internal.h"
#include "c_emit.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

int calynda_compile_to_c(const char *path, FILE *out) {
    char *source;
    Parser parser;
    AstProgram program;
    SymbolTable symbols;
    TypeChecker checker;
    HirProgram hir_program;
    const ParserError *parse_error;
    const SymbolTableError *symbol_error;
    const TypeCheckError *type_error;
    char diagnostic[256];
    int exit_code = 0;

    if (!path || !out) {
        return 1;
    }

    source = calynda_read_entire_file(path);
    if (!source) {
        return 66;
    }

    symbol_table_init(&symbols);
    type_checker_init(&checker);
    hir_program_init(&hir_program);
    parser_init(&parser, source);

    if (!parser_parse_program(&parser, &program)) {
        parse_error = parser_get_error(&parser);
        if (parse_error) {
            fprintf(stderr,
                    "%s:%d:%d: parse error: %s\n",
                    path,
                    parse_error->token.line,
                    parse_error->token.column,
                    parse_error->message);
        }
        parser_free(&parser);
        free(source);
        return 1;
    }

    if (!symbol_table_build(&symbols, &program)) {
        symbol_error = symbol_table_get_error(&symbols);
        if (symbol_error && symbol_table_format_error(symbol_error, diagnostic, sizeof(diagnostic))) {
            fprintf(stderr, "%s: semantic error: %s\n", path, diagnostic);
        } else {
            fprintf(stderr, "%s: semantic error while building symbols\n", path);
        }
        exit_code = 1;
        goto cleanup;
    }
    if (!type_checker_check_program(&checker, &program, &symbols)) {
        type_error = type_checker_get_error(&checker);
        if (type_error && type_checker_format_error(type_error, diagnostic, sizeof(diagnostic))) {
            fprintf(stderr, "%s: type error: %s\n", path, diagnostic);
        } else {
            fprintf(stderr, "%s: type error while checking program\n", path);
        }
        exit_code = 1;
        goto cleanup;
    }
    if (!hir_build_program(&hir_program, &program, &symbols, &checker)) {
        const HirBuildError *hir_error = hir_get_error(&hir_program);
        if (hir_error && hir_format_error(hir_error, diagnostic, sizeof(diagnostic))) {
            fprintf(stderr, "%s: HIR error: %s\n", path, diagnostic);
        } else {
            fprintf(stderr, "%s: HIR lowering failed\n", path);
        }
        exit_code = 1;
        goto cleanup;
    }
    if (!c_emit_program_with_file(out, &hir_program, path)) {
        fprintf(stderr, "%s: C emission failed\n", path);
        exit_code = 1;
    }

cleanup:
    hir_program_free(&hir_program);
    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
    free(source);
    return exit_code;
}
