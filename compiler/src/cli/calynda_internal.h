#ifndef CALYNDA_INTERNAL_H
#define CALYNDA_INTERNAL_H

#include "parser.h"

#include <limits.h>
#include <stdio.h>
#include <stdbool.h>

/* calynda_compile.c */
int calynda_compile_to_c(const char *path, FILE *out);

/* calynda_utils.c */
char *calynda_read_entire_file(const char *path);
bool calynda_executable_directory(char *buffer, size_t buffer_size);
bool calynda_write_temp_file(const char *prefix,
                             const char *contents,
                             char *path_buffer,
                             size_t buffer_size);
int calynda_run_c_compiler(const char *c_source_path,
                           const char *runtime_include_dir,
                           const char *runtime_lib_dir,
                           const char *output_path,
                           const char *target);
int calynda_run_child_process(const char *path, char *const argv[]);

#endif
