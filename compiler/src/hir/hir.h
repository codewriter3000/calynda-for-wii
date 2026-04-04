#ifndef CALYNDA_HIR_H
#define CALYNDA_HIR_H

#include "hir_types.h"

void hir_program_init(HirProgram *program);
void hir_program_free(HirProgram *program);
void hir_block_free(HirBlock *block);
void hir_statement_free(HirStatement *statement);
void hir_expression_free(HirExpression *expression);

bool hir_build_program(HirProgram *program,
                       const AstProgram *ast_program,
                       const SymbolTable *symbols,
                       const TypeChecker *checker);

const HirBuildError *hir_get_error(const HirProgram *program);
bool hir_format_error(const HirBuildError *error,
                      char *buffer,
                      size_t buffer_size);

#endif /* CALYNDA_HIR_H */