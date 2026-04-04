#ifndef C_EMIT_H
#define C_EMIT_H

#include "hir.h"

#include <stdbool.h>
#include <stdio.h>

/* Emit a complete C translation unit for the given HIR program. */
bool c_emit_program(FILE *out, const HirProgram *program);

#endif /* C_EMIT_H */
