#ifndef C_EMIT_INTERNAL_H
#define C_EMIT_INTERNAL_H

#include "c_emit.h"
#include "hir.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

/* Lambda table entry built during the pre-scan pass. */
typedef struct {
    const HirLambdaExpression *lambda;
    int                        id;
} CEmitLambdaEntry;

typedef struct {
    FILE              *out;
    int                indent;
    int                lambda_counter;
    /* Pre-scanned lambdas */
    CEmitLambdaEntry  *lambdas;
    size_t             lambda_count;
    size_t             lambda_capacity;
    /* Temp-arg counter for call expressions */
    int                call_tmp_counter;
} CEmitContext;

/* c_emit_names.c */
void c_emit_safe_name(FILE *out, const char *name);
void c_emit_mangle(FILE *out, const char *qualified_name);
void c_emit_lambda_name(FILE *out, int id);
void c_emit_closure_struct_name(FILE *out, int id);
void c_emit_global_name(FILE *out, const char *name);

/* c_emit_type.c */
void c_emit_type(FILE *out, CheckedType type);
void c_emit_type_str(char *buf, size_t bufsz, CheckedType type);

/* c_emit_closure.c */
bool c_emit_prescan_expr(CEmitContext *ctx, const HirExpression *expr);
bool c_emit_prescan_block(CEmitContext *ctx, const HirBlock *block);
bool c_emit_prescan_program(CEmitContext *ctx, const HirProgram *program);
int  c_emit_lambda_id(const CEmitContext *ctx, const HirLambdaExpression *lambda);
bool c_emit_lambda_functions(CEmitContext *ctx);

/* c_emit_expr.c */
bool c_emit_expr(CEmitContext *ctx, const HirExpression *expr);

/* c_emit_stmt.c */
bool c_emit_stmt(CEmitContext *ctx, const HirStatement *stmt);
bool c_emit_block(CEmitContext *ctx, const HirBlock *block);

#endif /* C_EMIT_INTERNAL_H */
