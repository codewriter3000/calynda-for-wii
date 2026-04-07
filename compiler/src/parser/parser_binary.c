#include "parser_internal.h"

static const BinaryOperatorMapping logical_or_operators[] = {
    {TOK_LOGIC_OR, AST_BINARY_OP_LOGICAL_OR}
};

static const BinaryOperatorMapping logical_and_operators[] = {
    {TOK_LOGIC_AND, AST_BINARY_OP_LOGICAL_AND}
};

static const BinaryOperatorMapping bitwise_or_operators[] = {
    {TOK_PIPE, AST_BINARY_OP_BIT_OR}
};

static const BinaryOperatorMapping bitwise_nand_operators[] = {
    {TOK_TILDE_AMP, AST_BINARY_OP_BIT_NAND}
};

static const BinaryOperatorMapping bitwise_xor_operators[] = {
    {TOK_CARET, AST_BINARY_OP_BIT_XOR}
};

static const BinaryOperatorMapping bitwise_xnor_operators[] = {
    {TOK_TILDE_CARET, AST_BINARY_OP_BIT_XNOR}
};

static const BinaryOperatorMapping bitwise_and_operators[] = {
    {TOK_AMP, AST_BINARY_OP_BIT_AND}
};

static const BinaryOperatorMapping equality_operators[] = {
    {TOK_EQ, AST_BINARY_OP_EQUAL},
    {TOK_NEQ, AST_BINARY_OP_NOT_EQUAL}
};

static const BinaryOperatorMapping relational_operators[] = {
    {TOK_LT, AST_BINARY_OP_LESS},
    {TOK_GT, AST_BINARY_OP_GREATER},
    {TOK_LTE, AST_BINARY_OP_LESS_EQUAL},
    {TOK_GTE, AST_BINARY_OP_GREATER_EQUAL}
};

static const BinaryOperatorMapping shift_operators[] = {
    {TOK_LSHIFT, AST_BINARY_OP_SHIFT_LEFT},
    {TOK_RSHIFT, AST_BINARY_OP_SHIFT_RIGHT}
};

static const BinaryOperatorMapping additive_operators[] = {
    {TOK_PLUS, AST_BINARY_OP_ADD},
    {TOK_MINUS, AST_BINARY_OP_SUBTRACT}
};

static const BinaryOperatorMapping multiplicative_operators[] = {
    {TOK_STAR, AST_BINARY_OP_MULTIPLY},
    {TOK_SLASH, AST_BINARY_OP_DIVIDE},
    {TOK_PERCENT, AST_BINARY_OP_MODULO}
};

AstExpression *parse_logical_or_expression(Parser *parser) {
    return parser_parse_binary_level(parser, parse_logical_and_expression,
                                     logical_or_operators,
                                     sizeof(logical_or_operators) /
                                         sizeof(logical_or_operators[0]));
}

AstExpression *parse_logical_and_expression(Parser *parser) {
    return parser_parse_binary_level(parser, parse_bitwise_or_expression,
                                     logical_and_operators,
                                     sizeof(logical_and_operators) /
                                         sizeof(logical_and_operators[0]));
}

AstExpression *parse_bitwise_or_expression(Parser *parser) {
    return parser_parse_binary_level(parser, parse_bitwise_nand_expression,
                                     bitwise_or_operators,
                                     sizeof(bitwise_or_operators) /
                                         sizeof(bitwise_or_operators[0]));
}

AstExpression *parse_bitwise_nand_expression(Parser *parser) {
    return parser_parse_binary_level(parser, parse_bitwise_xor_expression,
                                     bitwise_nand_operators,
                                     sizeof(bitwise_nand_operators) /
                                         sizeof(bitwise_nand_operators[0]));
}

AstExpression *parse_bitwise_xor_expression(Parser *parser) {
    return parser_parse_binary_level(parser, parse_bitwise_xnor_expression,
                                     bitwise_xor_operators,
                                     sizeof(bitwise_xor_operators) /
                                         sizeof(bitwise_xor_operators[0]));
}

AstExpression *parse_bitwise_xnor_expression(Parser *parser) {
    return parser_parse_binary_level(parser, parse_bitwise_and_expression,
                                     bitwise_xnor_operators,
                                     sizeof(bitwise_xnor_operators) /
                                         sizeof(bitwise_xnor_operators[0]));
}

AstExpression *parse_bitwise_and_expression(Parser *parser) {
    return parser_parse_binary_level(parser, parse_equality_expression,
                                     bitwise_and_operators,
                                     sizeof(bitwise_and_operators) /
                                         sizeof(bitwise_and_operators[0]));
}

AstExpression *parse_equality_expression(Parser *parser) {
    return parser_parse_binary_level(parser, parse_relational_expression,
                                     equality_operators,
                                     sizeof(equality_operators) /
                                         sizeof(equality_operators[0]));
}

AstExpression *parse_relational_expression(Parser *parser) {
    return parser_parse_binary_level(parser, parse_shift_expression,
                                     relational_operators,
                                     sizeof(relational_operators) /
                                         sizeof(relational_operators[0]));
}

AstExpression *parse_shift_expression(Parser *parser) {
    return parser_parse_binary_level(parser, parse_additive_expression,
                                     shift_operators,
                                     sizeof(shift_operators) /
                                         sizeof(shift_operators[0]));
}

AstExpression *parse_additive_expression(Parser *parser) {
    return parser_parse_binary_level(parser, parse_multiplicative_expression,
                                     additive_operators,
                                     sizeof(additive_operators) /
                                         sizeof(additive_operators[0]));
}

AstExpression *parse_multiplicative_expression(Parser *parser) {
    return parser_parse_binary_level(parser, parse_unary_expression,
                                     multiplicative_operators,
                                     sizeof(multiplicative_operators) /
                                         sizeof(multiplicative_operators[0]));
}

AstExpression *parser_parse_binary_level(Parser *parser,
                                         ParseExpressionFn operand_parser,
                                         const BinaryOperatorMapping *mappings,
                                         size_t mapping_count) {
    AstExpression *expression = operand_parser(parser);

    if (!expression) {
        return NULL;
    }

    for (;;) {
        size_t i;
        const BinaryOperatorMapping *matched = NULL;

        for (i = 0; i < mapping_count; i++) {
            if (parser_check(parser, mappings[i].token_type)) {
                matched = &mappings[i];
                break;
            }
        }

        if (!matched) {
            break;
        }

        parser_advance(parser);

        {
            AstExpression *binary = ast_expression_new(AST_EXPR_BINARY);
            AstExpression *right;

            if (!binary) {
                parser_set_oom_error(parser);
                ast_expression_free(expression);
                return NULL;
            }

            right = operand_parser(parser);
            if (!right) {
                ast_expression_free(binary);
                ast_expression_free(expression);
                return NULL;
            }

            binary->source_span = expression->source_span;
            binary->as.binary.operator = matched->operator;
            binary->as.binary.left = expression;
            binary->as.binary.right = right;
            expression = binary;
        }
    }

    return expression;
}
