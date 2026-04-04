#ifndef C_EMIT_H
#define C_EMIT_H

#include "hir.h"

#include <stdbool.h>
#include <stdio.h>

/* Emit a complete C translation unit for the given HIR program.
 * source_file may be NULL; if provided, #line directives will reference it. */
bool c_emit_program(FILE *out, const HirProgram *program);
bool c_emit_program_with_file(FILE *out, const HirProgram *program,
                              const char *source_file);

#endif /* C_EMIT_H */
