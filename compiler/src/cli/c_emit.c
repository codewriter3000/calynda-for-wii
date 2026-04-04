#include "c_emit.h"

#include <stddef.h>

/* Phase 1 stub: C emission is not yet implemented. */
bool c_emit_program(FILE *out, const HirProgram *program) {
    (void)program;
    fprintf(out, "#include <stdlib.h>\n");
    fprintf(out, "int main(void) { return 0; }\n");
    return true;
}
