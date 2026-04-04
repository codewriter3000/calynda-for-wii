#include "parser_internal.h"

void parser_init(Parser *parser, const char *source) {
    Token token;

    if (!parser) {
        return;
    }

    memset(parser, 0, sizeof(*parser));
    tokenizer_init(&parser->tokenizer, source ? source : "");

    do {
        token = tokenizer_next(&parser->tokenizer);
        if (token.type == TOK_ERROR) {
            parser_set_error(parser, token, "%.*s",
                             (int)token.length, token.start);
            break;
        }

        if (!parser_append_token(parser, token)) {
            parser_set_oom_error(parser);
            break;
        }
    } while (token.type != TOK_EOF);
}

void parser_free(Parser *parser) {
    if (!parser) {
        return;
    }

    free(parser->tokens);
    memset(parser, 0, sizeof(*parser));
}

bool parser_parse_program(Parser *parser, AstProgram *program) {
    if (!parser || !program) {
        return false;
    }

    ast_program_init(program);

    if (parser->has_error) {
        return false;
    }

    parser->current = 0;

    if (parser_match(parser, TOK_PACKAGE)) {
        AstQualifiedName package_name;

        if (!parser_parse_qualified_name(parser, &package_name)) {
            return false;
        }

        if (!parser_consume(parser, TOK_SEMICOLON,
                            "Expected ';' after package declaration.")) {
            ast_qualified_name_free(&package_name);
            return false;
        }

        ast_program_set_package(program, &package_name);
    }

    while (parser_match(parser, TOK_IMPORT)) {
        AstImportDecl import_decl;

        ast_import_decl_init(&import_decl);
        import_decl.kind = AST_IMPORT_PLAIN;

        if (!parser_parse_qualified_name(parser, &import_decl.module_name)) {
            ast_import_decl_free(&import_decl);
            ast_program_free(program);
            return false;
        }

        if (parser_match(parser, TOK_AS)) {
            /* import foo.bar as baz; */
            import_decl.kind = AST_IMPORT_ALIAS;
            import_decl.alias =
                parser_consume_identifier(parser, "Expected alias name after 'as'.");
            if (!import_decl.alias) {
                ast_import_decl_free(&import_decl);
                ast_program_free(program);
                return false;
            }
        } else if (parser_match(parser, TOK_DOT)) {
            if (parser_match(parser, TOK_STAR)) {
                /* import foo.bar.*; */
                import_decl.kind = AST_IMPORT_WILDCARD;
            } else if (parser_match(parser, TOK_LBRACE)) {
                /* import foo.bar.{a, b, c}; */
                import_decl.kind = AST_IMPORT_SELECTIVE;
                do {
                    char *selected = parser_consume_identifier(
                        parser, "Expected identifier in selective import.");
                    if (!selected) {
                        ast_import_decl_free(&import_decl);
                        ast_program_free(program);
                        return false;
                    }
                    if (!ast_import_decl_add_selected(&import_decl, selected)) {
                        free(selected);
                        parser_set_oom_error(parser);
                        ast_import_decl_free(&import_decl);
                        ast_program_free(program);
                        return false;
                    }
                    free(selected);
                } while (parser_match(parser, TOK_COMMA));

                if (!parser_consume(parser, TOK_RBRACE,
                                    "Expected '}' after selective import list.")) {
                    ast_import_decl_free(&import_decl);
                    ast_program_free(program);
                    return false;
                }
            } else {
                parser_set_error(parser, *parser_current_token(parser),
                                 "Expected '*' or '{' after '.' in import declaration.");
                ast_import_decl_free(&import_decl);
                ast_program_free(program);
                return false;
            }
        }

        if (!parser_consume(parser, TOK_SEMICOLON,
                            "Expected ';' after import declaration.")) {
            ast_import_decl_free(&import_decl);
            ast_program_free(program);
            return false;
        }

        if (!parser_add_import(parser, program, &import_decl)) {
            ast_program_free(program);
            return false;
        }
    }

    while (!parser_check(parser, TOK_EOF)) {
        AstTopLevelDecl *decl = parse_top_level_decl(parser);

        if (!decl) {
            ast_program_free(program);
            return false;
        }

        if (!parser_add_top_level_decl(parser, program, decl)) {
            ast_program_free(program);
            return false;
        }
    }

    return !parser->has_error;
}

AstExpression *parser_parse_expression(Parser *parser) {
    AstExpression *expression;

    if (!parser || parser->has_error) {
        return NULL;
    }

    parser->current = 0;
    expression = parse_expression_node(parser);
    if (!expression) {
        return NULL;
    }

    if (!parser_check(parser, TOK_EOF)) {
        parser_set_error(parser, *parser_current_token(parser),
                         "Expected end of expression.");
        ast_expression_free(expression);
        return NULL;
    }

    return expression;
}

const ParserError *parser_get_error(const Parser *parser) {
    if (!parser || !parser->has_error) {
        return NULL;
    }
    return &parser->error;
}
