#include "c_emit_internal.h"

#include <stdio.h>
#include <string.h>

static void emit_line_directive(CEmitContext *ctx, AstSourceSpan span) {
    if (!ctx->source_file || span.start_line <= 0) {
        return;
    }
    fprintf(ctx->out, "#line %d \"%s\"\n", span.start_line, ctx->source_file);
}

bool c_emit_stmt(CEmitContext *ctx, const HirStatement *stmt) {
    FILE *out = ctx->out;

    if (!stmt) {
        return true;
    }

    emit_line_directive(ctx, stmt->source_span);

    switch (stmt->kind) {

    case HIR_STMT_LOCAL_BINDING: {
        const HirLocalBinding *b = &stmt->as.local_binding;

        fputs("    CalyndaRtWord ", out);
        c_emit_global_name(out, b->name);
        if (b->initializer) {
            fputs(" = ", out);
            if (!c_emit_expr(ctx, b->initializer)) {
                return false;
            }
        } else {
            fputs(" = (CalyndaRtWord)0", out);
        }
        fputs(";\n", out);
        return true;
    }

    case HIR_STMT_RETURN:
        fputs("    return ", out);
        if (stmt->as.return_expression) {
            if (!c_emit_expr(ctx, stmt->as.return_expression)) {
                return false;
            }
        } else if (!ctx->is_boot_context) {
            /* boot() declares void return; bare 'return;' compiles without a value.
             * For start() and lambdas (CalyndaRtWord return), emit an explicit zero. */
            fputs("(CalyndaRtWord)0", out);
        }
        fputs(";\n", out);
        return true;

    case HIR_STMT_EXIT:
        fputs("    exit(0);\n", out);
        return true;

    case HIR_STMT_THROW:
        fputs("    __calynda_rt_throw(", out);
        if (stmt->as.throw_expression) {
            if (!c_emit_expr(ctx, stmt->as.throw_expression)) {
                return false;
            }
        } else {
            fputs("(CalyndaRtWord)0", out);
        }
        fputs(");\n", out);
        return true;

    case HIR_STMT_EXPRESSION:
        fputs("    (void)(", out);
        if (stmt->as.expression) {
            if (!c_emit_expr(ctx, stmt->as.expression)) {
                return false;
            }
        }
        fputs(");\n", out);
        return true;
    }

    return true;
}

bool c_emit_block(CEmitContext *ctx, const HirBlock *block) {
    size_t i;

    if (!block) {
        return true;
    }
    for (i = 0; i < block->statement_count; i++) {
        if (!c_emit_stmt(ctx, block->statements[i])) {
            return false;
        }
    }
    return true;
}
