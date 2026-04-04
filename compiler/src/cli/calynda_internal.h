#ifndef CALYNDA_INTERNAL_H
#define CALYNDA_INTERNAL_H

#include "asm_emit.h"
#include "bytecode.h"
#include "parser.h"

#include <limits.h>
#include <stdio.h>
#include <stdbool.h>

typedef enum {
    CALYNDA_EMIT_MODE_ASM = 0,
    CALYNDA_EMIT_MODE_BYTECODE
} CalyndaEmitMode;

/* calynda_compile.c */
int calynda_compile_to_machine_program(const char *path,
                                       MachineProgram *machine_program);
int calynda_compile_to_bytecode_program(const char *path,
                                        BytecodeProgram *bytecode_program);

/* calynda_utils.c */
char *calynda_read_entire_file(const char *path);
bool calynda_executable_directory(char *buffer, size_t buffer_size);
bool calynda_write_temp_file(const char *prefix,
                             const char *contents,
                             char *path_buffer,
                             size_t buffer_size);
int calynda_run_linker(const char *assembly_path,
                       const char *runtime_object_path,
                       const char *output_path);
int calynda_run_child_process(const char *path, char *const argv[]);

#endif
