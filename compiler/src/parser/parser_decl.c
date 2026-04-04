#include "parser_internal.h"

bool parser_parse_lambda_body(Parser *parser, AstLambdaBody *body) {
    if (parser_check(parser, TOK_LBRACE)) {
        AstBlock *block = parse_block(parser);

        if (!block) {
            return false;
        }

        body->kind = AST_LAMBDA_BODY_BLOCK;
        body->as.block = block;
        return true;
    }

    body->kind = AST_LAMBDA_BODY_EXPRESSION;
    body->as.expression = parse_assignment_expression(parser);
    return body->as.expression != NULL;
}

bool parser_parse_block_or_expression_body(Parser *parser,
                                           AstLambdaBody *body) {
    if (parser_check(parser, TOK_LBRACE)) {
        AstBlock *block = parse_block(parser);

        if (!block) {
            return false;
        }

        body->kind = AST_LAMBDA_BODY_BLOCK;
        body->as.block = block;
        return true;
    }

    body->kind = AST_LAMBDA_BODY_EXPRESSION;
    body->as.expression = parse_expression_node(parser);
    return body->as.expression != NULL;
}

AstTopLevelDecl *parse_top_level_decl(Parser *parser) {
    if (parser_check(parser, TOK_START)) {
        return parse_start_decl(parser);
    }

    if (parser_check(parser, TOK_BOOT)) {
        return parse_boot_decl(parser);
    }

    if (parser_check(parser, TOK_EXTERN)) {
        return parse_extern_decl(parser);
    }

    /* Peek past any modifier tokens to decide binding vs union. */
    if (parser_check(parser, TOK_PUBLIC) || parser_check(parser, TOK_PRIVATE) ||
        parser_check(parser, TOK_FINAL) || parser_check(parser, TOK_EXPORT) ||
        parser_check(parser, TOK_STATIC) || parser_check(parser, TOK_INTERNAL)) {
        size_t ahead = parser->current;
        while (true) {
            TokenType t = parser_token_at(parser, ahead)->type;
            if (t == TOK_PUBLIC || t == TOK_PRIVATE || t == TOK_FINAL ||
                t == TOK_EXPORT || t == TOK_STATIC || t == TOK_INTERNAL) {
                ahead++;
            } else {
                break;
            }
        }
        if (parser_token_at(parser, ahead)->type == TOK_UNION) {
            return parse_union_decl(parser);
        }
        return parse_binding_decl(parser);
    }

    if (parser_check(parser, TOK_VAR) ||
        is_type_start_token(parser_current_token(parser)->type)) {
        return parse_binding_decl(parser);
    }

    if (parser_check(parser, TOK_UNION)) {
        return parse_union_decl(parser);
    }

    parser_set_error(parser, *parser_current_token(parser),
                     "Expected top-level declaration.");
    return NULL;
}

AstTopLevelDecl *parse_start_decl(Parser *parser) {
    AstTopLevelDecl *decl = ast_top_level_decl_new(AST_TOP_LEVEL_START);

    if (!decl) {
        parser_set_oom_error(parser);
        return NULL;
    }

    decl->as.start_decl.start_span = parser_source_span(parser_current_token(parser));

    if (!parser_consume(parser, TOK_START, "Expected 'start'.") ||
        !parser_consume(parser, TOK_LPAREN, "Expected '(' after 'start'.") ||
        !parser_parse_parameter_list(parser, &decl->as.start_decl.parameters, false) ||
        !parser_consume(parser, TOK_RPAREN, "Expected ')' after start parameters.") ||
        !parser_consume(parser, TOK_ARROW, "Expected '->' after start parameters.") ||
        !parser_parse_block_or_expression_body(parser, &decl->as.start_decl.body) ||
        !parser_consume(parser, TOK_SEMICOLON, "Expected ';' after start declaration.")) {
        ast_top_level_decl_free(decl);
        return NULL;
    }

    return decl;
}

AstTopLevelDecl *parse_boot_decl(Parser *parser) {
    AstTopLevelDecl *decl = ast_top_level_decl_new(AST_TOP_LEVEL_BOOT);

    if (!decl) {
        parser_set_oom_error(parser);
        return NULL;
    }

    decl->as.boot_decl.boot_span = parser_source_span(parser_current_token(parser));

    if (!parser_consume(parser, TOK_BOOT, "Expected 'boot'.") ||
        !parser_consume(parser, TOK_LPAREN, "Expected '(' after 'boot'.") ||
        !parser_parse_parameter_list(parser, &decl->as.boot_decl.parameters, true) ||
        !parser_consume(parser, TOK_RPAREN, "Expected ')' after boot parameters.") ||
        !parser_consume(parser, TOK_ARROW, "Expected '->' after boot parameters.") ||
        !parser_parse_block_or_expression_body(parser, &decl->as.boot_decl.body) ||
        !parser_consume(parser, TOK_SEMICOLON, "Expected ';' after boot declaration.")) {
        ast_top_level_decl_free(decl);
        return NULL;
    }

    return decl;
}

AstTopLevelDecl *parse_extern_decl(Parser *parser) {
    AstTopLevelDecl *decl = ast_top_level_decl_new(AST_TOP_LEVEL_EXTERN);
    const Token     *name_token;
    char            *name;

    if (!decl) {
        parser_set_oom_error(parser);
        return NULL;
    }

    /* extern <ReturnType> <Name> = c(<ParamList>); */
    if (!parser_consume(parser, TOK_EXTERN, "Expected 'extern'.")) {
        ast_top_level_decl_free(decl);
        return NULL;
    }

    if (!parser_parse_type(parser, &decl->as.extern_decl.return_type)) {
        ast_top_level_decl_free(decl);
        return NULL;
    }

    name_token = parser_current_token(parser);
    name = parser_consume_identifier(parser, "Expected C function name after extern return type.");
    if (!name) {
        ast_top_level_decl_free(decl);
        return NULL;
    }
    decl->as.extern_decl.name = name;
    decl->as.extern_decl.name_span = parser_source_span(name_token);

    if (!parser_consume(parser, TOK_ASSIGN, "Expected '=' after extern name.")) {
        ast_top_level_decl_free(decl);
        return NULL;
    }

    /* Expect literal identifier 'c' for the foreign signature tag */
    if (!parser_check(parser, TOK_IDENTIFIER) ||
        parser_current_token(parser)->length != 1 ||
        parser_current_token(parser)->start[0] != 'c') {
        parser_set_error(parser, *parser_current_token(parser),
                         "Expected 'c' to introduce an extern C signature.");
        ast_top_level_decl_free(decl);
        return NULL;
    }
    parser_advance(parser);

    if (!parser_consume(parser, TOK_LPAREN, "Expected '(' after 'c' in extern declaration.") ||
        !parser_parse_extern_param_list(parser, &decl->as.extern_decl.parameters) ||
        !parser_consume(parser, TOK_RPAREN, "Expected ')' after extern parameter list.") ||
        !parser_consume(parser, TOK_SEMICOLON, "Expected ';' after extern declaration.")) {
        ast_top_level_decl_free(decl);
        return NULL;
    }

    return decl;
}

AstTopLevelDecl *parse_binding_decl(Parser *parser) {
    AstTopLevelDecl *decl = ast_top_level_decl_new(AST_TOP_LEVEL_BINDING);
    const Token *name_token;

    if (!decl) {
        parser_set_oom_error(parser);
        return NULL;
    }

    while (parser_check(parser, TOK_PUBLIC) || parser_check(parser, TOK_PRIVATE) ||
           parser_check(parser, TOK_FINAL) || parser_check(parser, TOK_EXPORT) ||
           parser_check(parser, TOK_STATIC) || parser_check(parser, TOK_INTERNAL)) {
        AstModifier modifier;

        switch (parser_current_token(parser)->type) {
        case TOK_PUBLIC:
            modifier = AST_MODIFIER_PUBLIC;
            break;
        case TOK_PRIVATE:
            modifier = AST_MODIFIER_PRIVATE;
            break;
        case TOK_EXPORT:
            modifier = AST_MODIFIER_EXPORT;
            break;
        case TOK_STATIC:
            modifier = AST_MODIFIER_STATIC;
            break;
        case TOK_INTERNAL:
            modifier = AST_MODIFIER_INTERNAL;
            break;
        default:
            modifier = AST_MODIFIER_FINAL;
            break;
        }

        parser_advance(parser);
        if (!parser_add_modifier(parser, &decl->as.binding_decl, modifier)) {
            ast_top_level_decl_free(decl);
            return NULL;
        }
    }

    if (parser_match(parser, TOK_VAR)) {
        decl->as.binding_decl.is_inferred_type = true;
    } else if (!parser_parse_type(parser, &decl->as.binding_decl.declared_type)) {
        ast_top_level_decl_free(decl);
        return NULL;
    }

    name_token = parser_current_token(parser);
    decl->as.binding_decl.name =
        parser_consume_identifier(parser, "Expected binding name.");
    if (!decl->as.binding_decl.name) {
        ast_top_level_decl_free(decl);
        return NULL;
    }
    decl->as.binding_decl.name_span = parser_source_span(name_token);

    if (!parser_consume(parser, TOK_ASSIGN, "Expected '=' after binding name.")) {
        ast_top_level_decl_free(decl);
        return NULL;
    }

    decl->as.binding_decl.initializer = parse_expression_node(parser);
    if (!decl->as.binding_decl.initializer) {
        ast_top_level_decl_free(decl);
        return NULL;
    }

    if (!parser_consume(parser, TOK_SEMICOLON, "Expected ';' after binding declaration.")) {
        ast_top_level_decl_free(decl);
        return NULL;
    }

    return decl;
}

AstBlock *parse_block(Parser *parser) {
    AstBlock *block = ast_block_new();

    if (!block) {
        parser_set_oom_error(parser);
        return NULL;
    }

    if (!parser_consume(parser, TOK_LBRACE, "Expected '{' to start block.")) {
        ast_block_free(block);
        return NULL;
    }

    while (!parser_check(parser, TOK_RBRACE) && !parser_check(parser, TOK_EOF)) {
        AstStatement *statement = parse_statement(parser);

        if (!statement) {
            ast_block_free(block);
            return NULL;
        }

        if (!parser_add_statement(parser, block, statement)) {
            ast_block_free(block);
            return NULL;
        }
    }

    if (!parser_consume(parser, TOK_RBRACE, "Expected '}' after block.")) {
        ast_block_free(block);
        return NULL;
    }

    return block;
}
