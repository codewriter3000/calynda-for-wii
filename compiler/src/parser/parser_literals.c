#include "parser_internal.h"

bool parser_add_template_text(Parser *parser, AstLiteral *literal,
                              const Token *token) {
    char *text = copy_template_segment_text(token);
    bool ok;

    if (!text) {
        parser_set_oom_error(parser);
        return false;
    }

    ok = ast_template_literal_append_text(literal, text);
    free(text);

    if (!ok) {
        parser_set_oom_error(parser);
        return false;
    }

    return true;
}

bool parser_add_template_expression(Parser *parser, AstLiteral *literal,
                                    AstExpression *expression) {
    if (ast_template_literal_append_expression(literal, expression)) {
        return true;
    }

    parser_set_oom_error(parser);
    ast_expression_free(expression);
    return false;
}

AstExpression *parse_array_literal_expression(Parser *parser) {
    AstExpression *expression = ast_expression_new(AST_EXPR_ARRAY_LITERAL);

    if (!expression) {
        parser_set_oom_error(parser);
        return NULL;
    }

    expression->source_span = parser_source_span(parser_current_token(parser));

    if (!parser_consume(parser, TOK_LBRACKET, "Expected '[' to start array literal.")) {
        ast_expression_free(expression);
        return NULL;
    }

    if (!parser_check(parser, TOK_RBRACKET)) {
        do {
            AstExpression *element = parse_expression_node(parser);

            if (!element) {
                ast_expression_free(expression);
                return NULL;
            }

            if (!parser_add_expression(parser, &expression->as.array_literal.elements,
                                       element)) {
                ast_expression_free(expression);
                return NULL;
            }
        } while (parser_match(parser, TOK_COMMA));
    }

    if (!parser_consume(parser, TOK_RBRACKET, "Expected ']' after array literal.")) {
        ast_expression_free(expression);
        return NULL;
    }

    return expression;
}

AstExpression *parse_cast_expression(Parser *parser) {
    AstExpression *expression = ast_expression_new(AST_EXPR_CAST);

    if (!expression) {
        parser_set_oom_error(parser);
        return NULL;
    }

    expression->source_span = parser_source_span(parser_current_token(parser));

    expression->as.cast.target_type =
        primitive_type_from_token(parser_current_token(parser)->type);
    parser_advance(parser);

    if (!parser_consume(parser, TOK_LPAREN, "Expected '(' after cast type.")) {
        ast_expression_free(expression);
        return NULL;
    }

    expression->as.cast.expression = parse_expression_node(parser);
    if (!expression->as.cast.expression) {
        ast_expression_free(expression);
        return NULL;
    }

    if (!parser_consume(parser, TOK_RPAREN, "Expected ')' after cast expression.")) {
        ast_expression_free(expression);
        return NULL;
    }

    return expression;
}

AstExpression *parse_template_literal_expression(Parser *parser) {
    AstExpression *expression = ast_expression_new(AST_EXPR_LITERAL);

    if (!expression) {
        parser_set_oom_error(parser);
        return NULL;
    }

    expression->as.literal.kind = AST_LITERAL_TEMPLATE;
    expression->source_span = parser_source_span(parser_current_token(parser));

    if (parser_match(parser, TOK_TEMPLATE_FULL)) {
        if (!parser_add_template_text(parser, &expression->as.literal,
                                      parser_previous_token(parser))) {
            ast_expression_free(expression);
            return NULL;
        }
        return expression;
    }

    if (!parser_consume(parser, TOK_TEMPLATE_START,
                        "Expected template literal start token.")) {
        ast_expression_free(expression);
        return NULL;
    }

    if (!parser_add_template_text(parser, &expression->as.literal,
                                  parser_previous_token(parser))) {
        ast_expression_free(expression);
        return NULL;
    }

    for (;;) {
        AstExpression *interpolation = parse_expression_node(parser);

        if (!interpolation) {
            ast_expression_free(expression);
            return NULL;
        }

        if (!parser_add_template_expression(parser, &expression->as.literal,
                                            interpolation)) {
            ast_expression_free(expression);
            return NULL;
        }

        if (parser_match(parser, TOK_TEMPLATE_MIDDLE)) {
            if (!parser_add_template_text(parser, &expression->as.literal,
                                          parser_previous_token(parser))) {
                ast_expression_free(expression);
                return NULL;
            }
            continue;
        }

        if (parser_match(parser, TOK_TEMPLATE_END)) {
            if (!parser_add_template_text(parser, &expression->as.literal,
                                          parser_previous_token(parser))) {
                ast_expression_free(expression);
                return NULL;
            }
            break;
        }

        parser_set_error(parser, *parser_current_token(parser),
                         "Expected template continuation after interpolation.");
        ast_expression_free(expression);
        return NULL;
    }

    return expression;
}
