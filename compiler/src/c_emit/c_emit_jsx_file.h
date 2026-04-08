#ifndef C_EMIT_JSX_FILE_H
#define C_EMIT_JSX_FILE_H

/*
 * c_emit_jsx_file.h — Full-file JSX compiler.
 *
 * Alternative compilation path for Calynda source files that contain
 * JSX/Solid component declarations.  Bypasses the normal sema/HIR
 * pipeline and emits C code directly from JSX constructs.
 */

#include <stdbool.h>
#include <stdio.h>

/*
 * Compile a Calynda+JSX source file directly to C code.
 * Returns 0 on success, non-zero on failure.
 *
 * The source file may contain:
 *   - import io.gui;
 *   - component Name = () -> { signal ...; return (<JSX>); };
 *   - boot() -> { gui.mount(Name()); };
 */
int jsx_compile_to_c(const char *source, const char *path, FILE *out);

/*
 * Check whether a source string contains JSX component syntax
 * (i.e. the "component" keyword used as a declaration).
 */
bool jsx_source_has_components(const char *source);

#endif /* C_EMIT_JSX_FILE_H */
