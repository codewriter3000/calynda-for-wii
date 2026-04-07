#include "c_emit_internal.h"
#include "ast_types.h"

#include <stdio.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/* Helper: emit a binary operator symbol                                */
/* ------------------------------------------------------------------ */

static void emit_binary_op(FILE *out, AstBinaryOperator op) {
    switch (op) {
    case AST_BINARY_OP_LOGICAL_OR:    fputs("||", out);  return;
    case AST_BINARY_OP_LOGICAL_AND:   fputs("&&", out);  return;
    case AST_BINARY_OP_BIT_OR:        fputs("|",  out);  return;
    case AST_BINARY_OP_BIT_AND:       fputs("&",  out);  return;
    case AST_BINARY_OP_BIT_XOR:       fputs("^",  out);  return;
    case AST_BINARY_OP_BIT_NAND:      fputs("&~", out);  return;
    case AST_BINARY_OP_BIT_XNOR:      fputs("^~", out);  return;
    case AST_BINARY_OP_EQUAL:         fputs("==", out);  return;
    case AST_BINARY_OP_NOT_EQUAL:     fputs("!=", out);  return;
    case AST_BINARY_OP_LESS:          fputs("<",  out);  return;
    case AST_BINARY_OP_GREATER:       fputs(">",  out);  return;
    case AST_BINARY_OP_LESS_EQUAL:    fputs("<=", out);  return;
    case AST_BINARY_OP_GREATER_EQUAL: fputs(">=", out);  return;
    case AST_BINARY_OP_SHIFT_LEFT:    fputs("<<", out);  return;
    case AST_BINARY_OP_SHIFT_RIGHT:   fputs(">>", out);  return;
    case AST_BINARY_OP_ADD:           fputs("+",  out);  return;
    case AST_BINARY_OP_SUBTRACT:      fputs("-",  out);  return;
    case AST_BINARY_OP_MULTIPLY:      fputs("*",  out);  return;
    case AST_BINARY_OP_DIVIDE:        fputs("/",  out);  return;
    case AST_BINARY_OP_MODULO:        fputs("%",  out);  return;
    }
    fputs("+", out); /* fallback */
}

static void emit_assign_op(FILE *out, AstAssignmentOperator op) {
    switch (op) {
    case AST_ASSIGN_OP_ASSIGN:      fputs("=",   out); return;
    case AST_ASSIGN_OP_ADD:         fputs("+=",  out); return;
    case AST_ASSIGN_OP_SUBTRACT:    fputs("-=",  out); return;
    case AST_ASSIGN_OP_MULTIPLY:    fputs("*=",  out); return;
    case AST_ASSIGN_OP_DIVIDE:      fputs("/=",  out); return;
    case AST_ASSIGN_OP_MODULO:      fputs("%=",  out); return;
    case AST_ASSIGN_OP_BIT_AND:     fputs("&=",  out); return;
    case AST_ASSIGN_OP_BIT_OR:      fputs("|=",  out); return;
    case AST_ASSIGN_OP_BIT_XOR:     fputs("^=",  out); return;
    case AST_ASSIGN_OP_SHIFT_LEFT:  fputs("<<=", out); return;
    case AST_ASSIGN_OP_SHIFT_RIGHT: fputs(">>=", out); return;
    }
    fputs("=", out);
}

static void emit_unary_op(FILE *out, AstUnaryOperator op) {
    switch (op) {
    case AST_UNARY_OP_LOGICAL_NOT:   fputs("!",  out); return;
    case AST_UNARY_OP_BITWISE_NOT:   fputs("~",  out); return;
    case AST_UNARY_OP_NEGATE:        fputs("-",  out); return;
    case AST_UNARY_OP_PLUS:          fputs("+",  out); return;
    case AST_UNARY_OP_PRE_INCREMENT: fputs("++", out); return;
    case AST_UNARY_OP_PRE_DECREMENT: fputs("--", out); return;
    case AST_UNARY_OP_DEREF:         fputs("*",  out); return;
    case AST_UNARY_OP_ADDRESS_OF:    fputs("&",  out); return;
    }
}

/* ------------------------------------------------------------------ */
/* Main expression emitter                                              */
/* ------------------------------------------------------------------ */

bool c_emit_expr(CEmitContext *ctx, const HirExpression *expr) {
    FILE  *out = ctx->out;
    size_t i;

    if (!expr) {
        fputs("(CalyndaRtWord)0", out);
        return true;
    }

    switch (expr->kind) {

    /* ---- Literal ---- */
    case HIR_EXPR_LITERAL:
        switch (expr->as.literal.kind) {
        case AST_LITERAL_INTEGER:
            fprintf(out, "(CalyndaRtWord)%s", expr->as.literal.as.text);
            return true;
        case AST_LITERAL_FLOAT:
            fprintf(out, "(CalyndaRtWord)(uintptr_t)%s", expr->as.literal.as.text);
            return true;
        case AST_LITERAL_BOOL:
            fprintf(out, "(CalyndaRtWord)%d", expr->as.literal.as.bool_value ? 1 : 0);
            return true;
        case AST_LITERAL_CHAR:
            fprintf(out, "(CalyndaRtWord)'%s'", expr->as.literal.as.text);
            return true;
        case AST_LITERAL_STRING:
            fputs("calynda_rt_make_string_copy(\"", out);
            {
                const char *p;
                for (p = expr->as.literal.as.text; *p; p++) {
                    if (*p == '"' || *p == '\\') {
                        fputc('\\', out);
                    }
                    fputc(*p, out);
                }
            }
            fputs("\")", out);
            return true;
        case AST_LITERAL_NULL:
            fputs("(CalyndaRtWord)0", out);
            return true;
        case AST_LITERAL_TEMPLATE:
            /* Should not appear here; templates use HIR_EXPR_TEMPLATE */
            fputs("(CalyndaRtWord)0 /* template literal */", out);
            return true;
        }
        fputs("(CalyndaRtWord)0", out);
        return true;

    /* ---- Template ---- */
    case HIR_EXPR_TEMPLATE: {
        const HirTemplatePartList *parts = &expr->as.template_parts;

        fputs("__calynda_rt_template_build(", out);
        fprintf(out, "%zu, (CalyndaRtTemplatePart[]){", parts->count);
        for (i = 0; i < parts->count; i++) {
            const HirTemplatePart *p = &parts->items[i];

            if (i > 0) {
                fputs(", ", out);
            }
            if (p->kind == AST_TEMPLATE_PART_TEXT) {
                fputs("{CALYNDA_RT_TEMPLATE_TAG_TEXT, (CalyndaRtWord)(uintptr_t)\"", out);
                {
                    const char *c;
                    for (c = p->as.text; *c; c++) {
                        if (*c == '"' || *c == '\\') {
                            fputc('\\', out);
                        }
                        fputc(*c, out);
                    }
                }
                fputs("\"}", out);
            } else {
                fputs("{CALYNDA_RT_TEMPLATE_TAG_VALUE, ", out);
                if (!c_emit_expr(ctx, p->as.expression)) {
                    return false;
                }
                fputs("}", out);
            }
        }
        fputs("})", out);
        return true;
    }

    /* ---- Symbol reference ---- */
    case HIR_EXPR_SYMBOL:
        switch (expr->as.symbol.kind) {
        case SYMBOL_KIND_TOP_LEVEL_BINDING:
        case SYMBOL_KIND_PARAMETER:
        case SYMBOL_KIND_LOCAL:
            c_emit_global_name(out, expr->as.symbol.name);
            return true;
        case SYMBOL_KIND_EXTERN:
            /* Extern C symbols use the raw C name without __cal_ prefix */
            c_emit_safe_name(out, expr->as.symbol.name);
            return true;
        default:
            c_emit_global_name(out, expr->as.symbol.name);
            return true;
        }

    /* ---- Lambda ---- */
    case HIR_EXPR_LAMBDA: {
        int id = c_emit_lambda_id(ctx, &expr->as.lambda);

        if (id < 0) {
            fputs("(CalyndaRtWord)0 /* unregistered lambda */", out);
            return true;
        }
        fputs("__calynda_rt_closure_new(", out);
        c_emit_lambda_name(out, id);
        fputs(", 0, (CalyndaRtWord *)0)", out);
        return true;
    }

    /* ---- Assignment ---- */
    case HIR_EXPR_ASSIGNMENT:
        fputs("(", out);
        if (!c_emit_expr(ctx, expr->as.assignment.target)) {
            return false;
        }
        fputs(" ", out);
        emit_assign_op(out, expr->as.assignment.operator);
        fputs(" ", out);
        if (!c_emit_expr(ctx, expr->as.assignment.value)) {
            return false;
        }
        fputs(")", out);
        return true;

    /* ---- Ternary ---- */
    case HIR_EXPR_TERNARY:
        fputs("(", out);
        if (!c_emit_expr(ctx, expr->as.ternary.condition)) {
            return false;
        }
        fputs(" ? ", out);
        if (!c_emit_expr(ctx, expr->as.ternary.then_branch)) {
            return false;
        }
        fputs(" : ", out);
        if (!c_emit_expr(ctx, expr->as.ternary.else_branch)) {
            return false;
        }
        fputs(")", out);
        return true;

    /* ---- Binary ---- */
    case HIR_EXPR_BINARY:
        fputs("(CalyndaRtWord)((", out);
        if (!c_emit_expr(ctx, expr->as.binary.left)) {
            return false;
        }
        fputs(") ", out);
        emit_binary_op(out, expr->as.binary.operator);
        fputs(" (", out);
        if (!c_emit_expr(ctx, expr->as.binary.right)) {
            return false;
        }
        fputs("))", out);
        return true;

    /* ---- Unary ---- */
    case HIR_EXPR_UNARY:
        fputs("(CalyndaRtWord)(", out);
        emit_unary_op(out, expr->as.unary.operator);
        fputs("(", out);
        if (!c_emit_expr(ctx, expr->as.unary.operand)) {
            return false;
        }
        fputs("))", out);
        return true;

    /* ---- Call ---- */
    case HIR_EXPR_CALL: {
        size_t argc = expr->as.call.argument_count;
        bool is_extern_call = expr->as.call.callee &&
                              expr->as.call.callee->kind == HIR_EXPR_SYMBOL &&
                              expr->as.call.callee->as.symbol.kind == SYMBOL_KIND_EXTERN;

        if (is_extern_call) {
            /* Direct C function call for extern symbols */
            if (!c_emit_expr(ctx, expr->as.call.callee)) {
                return false;
            }
            fputs("(", out);
            for (i = 0; i < argc; i++) {
                if (i > 0) {
                    fputs(", ", out);
                }
                if (!c_emit_expr(ctx, expr->as.call.arguments[i])) {
                    return false;
                }
            }
            fputs(")", out);
        } else {
            fputs("__calynda_rt_call_callable(", out);
            if (!c_emit_expr(ctx, expr->as.call.callee)) {
                return false;
            }
            if (argc == 0) {
                fputs(", 0, (CalyndaRtWord *)0)", out);
            } else {
                fprintf(out, ", %zu, (CalyndaRtWord[]){", argc);
                for (i = 0; i < argc; i++) {
                    if (i > 0) {
                        fputs(", ", out);
                    }
                    if (!c_emit_expr(ctx, expr->as.call.arguments[i])) {
                        return false;
                    }
                }
                fputs("})", out);
            }
        }
        return true;
    }

    /* ---- Index ---- */
    case HIR_EXPR_INDEX:
        fputs("__calynda_rt_index_load(", out);
        if (!c_emit_expr(ctx, expr->as.index.target)) {
            return false;
        }
        fputs(", ", out);
        if (!c_emit_expr(ctx, expr->as.index.index)) {
            return false;
        }
        fputs(")", out);
        return true;

    /* ---- Member ---- */
    case HIR_EXPR_MEMBER:
        fputs("__calynda_rt_member_load(", out);
        if (!c_emit_expr(ctx, expr->as.member.target)) {
            return false;
        }
        fprintf(out, ", \"%s\")", expr->as.member.member ? expr->as.member.member : "");
        return true;

    /* ---- Cast ---- */
    case HIR_EXPR_CAST:
        fputs("__calynda_rt_cast_value(", out);
        if (!c_emit_expr(ctx, expr->as.cast.expression)) {
            return false;
        }
        /* Map the target type to a CalyndaRtTypeTag */
        {
            const CheckedType t = expr->as.cast.target_type;
            const char       *tag = "CALYNDA_RT_TYPE_RAW_WORD";

            if (t.kind == CHECKED_TYPE_VALUE) {
                switch (t.primitive) {
                case AST_PRIMITIVE_BOOL:
                    tag = "CALYNDA_RT_TYPE_BOOL"; break;
                case AST_PRIMITIVE_INT32: case AST_PRIMITIVE_INT:
                case AST_PRIMITIVE_INT8:  case AST_PRIMITIVE_INT16:
                case AST_PRIMITIVE_SHORT: case AST_PRIMITIVE_SBYTE:
                    tag = "CALYNDA_RT_TYPE_INT32"; break;
                case AST_PRIMITIVE_INT64: case AST_PRIMITIVE_LONG:
                    tag = "CALYNDA_RT_TYPE_INT64"; break;
                case AST_PRIMITIVE_STRING:
                    tag = "CALYNDA_RT_TYPE_STRING"; break;
                default:
                    tag = "CALYNDA_RT_TYPE_RAW_WORD"; break;
                }
            } else if (t.kind == CHECKED_TYPE_VOID) {
                tag = "CALYNDA_RT_TYPE_VOID";
            }
            fprintf(out, ", %s)", tag);
        }
        return true;

    /* ---- Array literal ---- */
    case HIR_EXPR_ARRAY_LITERAL: {
        size_t count = expr->as.array_literal.element_count;

        if (count == 0) {
            fputs("__calynda_rt_array_literal(0, (CalyndaRtWord *)0)", out);
            return true;
        }
        fprintf(out, "__calynda_rt_array_literal(%zu, (CalyndaRtWord[]){", count);
        for (i = 0; i < count; i++) {
            if (i > 0) {
                fputs(", ", out);
            }
            if (!c_emit_expr(ctx, expr->as.array_literal.elements[i])) {
                return false;
            }
        }
        fputs("})", out);
        return true;
    }

    /* ---- Discard ---- */
    case HIR_EXPR_DISCARD:
        fputs("(CalyndaRtWord)0", out);
        return true;

    /* ---- Post-increment / post-decrement ---- */
    case HIR_EXPR_POST_INCREMENT:
        fputs("(CalyndaRtWord)(", out);
        if (!c_emit_expr(ctx, expr->as.post_increment.operand)) {
            return false;
        }
        fputs("++)", out);
        return true;

    case HIR_EXPR_POST_DECREMENT:
        fputs("(CalyndaRtWord)(", out);
        if (!c_emit_expr(ctx, expr->as.post_decrement.operand)) {
            return false;
        }
        fputs("--)", out);
        return true;
    }

    fputs("(CalyndaRtWord)0 /* unknown expr */", out);
    return true;
}
