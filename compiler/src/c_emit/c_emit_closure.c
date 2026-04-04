#include "c_emit_internal.h"

#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/* Lambda pre-scan: walk HIR to collect all lambda expressions          */
/* ------------------------------------------------------------------ */

static bool prescan_register(CEmitContext *ctx, const HirLambdaExpression *lambda) {
    CEmitLambdaEntry *resized;
    size_t            new_cap;

    if (ctx->lambda_count >= ctx->lambda_capacity) {
        new_cap = (ctx->lambda_capacity == 0) ? 8 : ctx->lambda_capacity * 2;
        resized = realloc(ctx->lambdas, new_cap * sizeof(*ctx->lambdas));
        if (!resized) {
            return false;
        }
        ctx->lambdas         = resized;
        ctx->lambda_capacity = new_cap;
    }
    ctx->lambdas[ctx->lambda_count].lambda = lambda;
    ctx->lambdas[ctx->lambda_count].id     = ctx->lambda_counter++;
    ctx->lambda_count++;
    return true;
}

bool c_emit_prescan_expr(CEmitContext *ctx, const HirExpression *expr) {
    size_t i;

    if (!expr) {
        return true;
    }

    switch (expr->kind) {
    case HIR_EXPR_LAMBDA:
        if (!prescan_register(ctx, &expr->as.lambda)) {
            return false;
        }
        /* Recurse into lambda body */
        return c_emit_prescan_block(ctx, expr->as.lambda.body);

    case HIR_EXPR_BINARY:
        return c_emit_prescan_expr(ctx, expr->as.binary.left) &&
               c_emit_prescan_expr(ctx, expr->as.binary.right);

    case HIR_EXPR_UNARY:
        return c_emit_prescan_expr(ctx, expr->as.unary.operand);

    case HIR_EXPR_ASSIGNMENT:
        return c_emit_prescan_expr(ctx, expr->as.assignment.target) &&
               c_emit_prescan_expr(ctx, expr->as.assignment.value);

    case HIR_EXPR_TERNARY:
        return c_emit_prescan_expr(ctx, expr->as.ternary.condition) &&
               c_emit_prescan_expr(ctx, expr->as.ternary.then_branch) &&
               c_emit_prescan_expr(ctx, expr->as.ternary.else_branch);

    case HIR_EXPR_CALL:
        if (!c_emit_prescan_expr(ctx, expr->as.call.callee)) {
            return false;
        }
        for (i = 0; i < expr->as.call.argument_count; i++) {
            if (!c_emit_prescan_expr(ctx, expr->as.call.arguments[i])) {
                return false;
            }
        }
        return true;

    case HIR_EXPR_INDEX:
        return c_emit_prescan_expr(ctx, expr->as.index.target) &&
               c_emit_prescan_expr(ctx, expr->as.index.index);

    case HIR_EXPR_MEMBER:
        return c_emit_prescan_expr(ctx, expr->as.member.target);

    case HIR_EXPR_CAST:
        return c_emit_prescan_expr(ctx, expr->as.cast.expression);

    case HIR_EXPR_ARRAY_LITERAL:
        for (i = 0; i < expr->as.array_literal.element_count; i++) {
            if (!c_emit_prescan_expr(ctx, expr->as.array_literal.elements[i])) {
                return false;
            }
        }
        return true;

    case HIR_EXPR_TEMPLATE:
        for (i = 0; i < expr->as.template_parts.count; i++) {
            if (expr->as.template_parts.items[i].kind == AST_TEMPLATE_PART_EXPRESSION) {
                if (!c_emit_prescan_expr(ctx,
                                         expr->as.template_parts.items[i].as.expression)) {
                    return false;
                }
            }
        }
        return true;

    case HIR_EXPR_POST_INCREMENT:
        return c_emit_prescan_expr(ctx, expr->as.post_increment.operand);

    case HIR_EXPR_POST_DECREMENT:
        return c_emit_prescan_expr(ctx, expr->as.post_decrement.operand);

    case HIR_EXPR_LITERAL:
    case HIR_EXPR_SYMBOL:
    case HIR_EXPR_DISCARD:
        return true;
    }
    return true;
}

bool c_emit_prescan_block(CEmitContext *ctx, const HirBlock *block) {
    size_t i;

    if (!block) {
        return true;
    }
    for (i = 0; i < block->statement_count; i++) {
        const HirStatement *stmt = block->statements[i];

        if (!stmt) {
            continue;
        }
        switch (stmt->kind) {
        case HIR_STMT_LOCAL_BINDING:
            if (!c_emit_prescan_expr(ctx, stmt->as.local_binding.initializer)) {
                return false;
            }
            break;
        case HIR_STMT_RETURN:
            if (!c_emit_prescan_expr(ctx, stmt->as.return_expression)) {
                return false;
            }
            break;
        case HIR_STMT_THROW:
            if (!c_emit_prescan_expr(ctx, stmt->as.throw_expression)) {
                return false;
            }
            break;
        case HIR_STMT_EXPRESSION:
            if (!c_emit_prescan_expr(ctx, stmt->as.expression)) {
                return false;
            }
            break;
        case HIR_STMT_EXIT:
            break;
        }
    }
    return true;
}

bool c_emit_prescan_program(CEmitContext *ctx, const HirProgram *program) {
    size_t i;

    for (i = 0; i < program->top_level_count; i++) {
        const HirTopLevelDecl *decl = program->top_level_decls[i];

        if (!decl) {
            continue;
        }
        switch (decl->kind) {
        case HIR_TOP_LEVEL_BINDING:
            if (!c_emit_prescan_expr(ctx, decl->as.binding.initializer)) {
                return false;
            }
            break;
        case HIR_TOP_LEVEL_START:
            if (!c_emit_prescan_block(ctx, decl->as.start.body)) {
                return false;
            }
            break;
        case HIR_TOP_LEVEL_UNION:
            break;
        }
    }
    return true;
}

int c_emit_lambda_id(const CEmitContext *ctx, const HirLambdaExpression *lambda) {
    size_t i;

    for (i = 0; i < ctx->lambda_count; i++) {
        if (ctx->lambdas[i].lambda == lambda) {
            return ctx->lambdas[i].id;
        }
    }
    return -1;
}

/* ------------------------------------------------------------------ */
/* Emit all collected lambda functions                                  */
/* ------------------------------------------------------------------ */

bool c_emit_lambda_functions(CEmitContext *ctx) {
    size_t i, j;

    for (i = 0; i < ctx->lambda_count; i++) {
        const CEmitLambdaEntry     *entry  = &ctx->lambdas[i];
        const HirLambdaExpression  *lambda = entry->lambda;
        int                         id     = entry->id;

        /* Emit the C function signature */
        fprintf(ctx->out,
                "static CalyndaRtWord ");
        c_emit_lambda_name(ctx->out, id);
        fprintf(ctx->out,
                "(const CalyndaRtWord *__captures, size_t __capture_count,\n"
                " const CalyndaRtWord *__arguments, size_t __argument_count)\n"
                "{\n");

        /* Bind parameter names to argument slots */
        for (j = 0; j < lambda->parameters.count; j++) {
            fprintf(ctx->out, "    CalyndaRtWord ");
            c_emit_global_name(ctx->out, lambda->parameters.items[j].name);
            fprintf(ctx->out, " = __arguments[%zu];\n", j);
        }

        if (lambda->parameters.count == 0) {
            fprintf(ctx->out,
                    "    (void)__captures; (void)__capture_count;\n"
                    "    (void)__arguments; (void)__argument_count;\n");
        } else {
            fprintf(ctx->out,
                    "    (void)__captures; (void)__capture_count;\n"
                    "    (void)__argument_count;\n");
        }

        /* Emit body */
        if (!c_emit_block(ctx, lambda->body)) {
            return false;
        }

        fprintf(ctx->out, "    return (CalyndaRtWord)0;\n");
        fprintf(ctx->out, "}\n\n");
    }
    return true;
}
