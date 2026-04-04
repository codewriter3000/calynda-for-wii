#define _POSIX_C_SOURCE 200809L

#include "calynda_internal.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static bool parse_build_output(int argc,
                               char **argv,
                               const char *default_output,
                               const char **output_path);
static void print_usage(FILE *out, const char *program_name);
static int emit_program_file(const char *path, CalyndaEmitMode mode);
static int build_program_file(const char *source_path, const char *output_path);
static int run_program_file(const char *source_path, int argc, char **argv);

int main(int argc, char **argv) {
    const char *program_name = argc > 0 ? argv[0] : "calynda";

    if (argc < 2) {
        print_usage(stderr, program_name);
        return 64;
    }

    if (strcmp(argv[1], "help") == 0 || strcmp(argv[1], "--help") == 0 ||
        strcmp(argv[1], "-h") == 0) {
        print_usage(stdout, program_name);
        return 0;
    }
    if (strcmp(argv[1], "asm") == 0) {
        if (argc != 3) {
            print_usage(stderr, program_name);
            return 64;
        }
        return emit_program_file(argv[2], CALYNDA_EMIT_MODE_ASM);
    }
    if (strcmp(argv[1], "bytecode") == 0) {
        if (argc != 3) {
            print_usage(stderr, program_name);
            return 64;
        }
        return emit_program_file(argv[2], CALYNDA_EMIT_MODE_BYTECODE);
    }
    if (strcmp(argv[1], "build") == 0) {
        const char *output_path;
        char default_output[PATH_MAX];
        const char *source_path;
        const char *slash;
        const char *basename;
        size_t length;

        if (argc < 3) {
            print_usage(stderr, program_name);
            return 64;
        }

        source_path = argv[2];
        slash = strrchr(source_path, '/');
        basename = slash ? slash + 1 : source_path;
        length = strlen(basename);
        if (length > 4 && strcmp(basename + length - 4, ".cal") == 0) {
            length -= 4;
        }
        if (snprintf(default_output,
                     sizeof(default_output),
                     "build/%.*s",
                     (int)length,
                     basename) < 0) {
            fprintf(stderr, "failed to build default output path\n");
            return 1;
        }
        if (!parse_build_output(argc - 3, argv + 3, default_output, &output_path)) {
            print_usage(stderr, program_name);
            return 64;
        }
        return build_program_file(source_path, output_path);
    }
    if (strcmp(argv[1], "run") == 0) {
        if (argc < 3) {
            print_usage(stderr, program_name);
            return 64;
        }
        return run_program_file(argv[2], argc - 3, argv + 3);
    }

    print_usage(stderr, program_name);
    return 64;
}

static bool parse_build_output(int argc,
                               char **argv,
                               const char *default_output,
                               const char **output_path) {
    if (!output_path || !default_output) {
        return false;
    }

    *output_path = default_output;
    if (argc == 0) {
        return true;
    }
    if (argc == 2 && strcmp(argv[0], "-o") == 0) {
        *output_path = argv[1];
        return true;
    }
    return false;
}

static void print_usage(FILE *out, const char *program_name) {
    fprintf(out, "Usage: %s <command> ...\n", program_name);
    fprintf(out, "\nCommands:\n");
    fprintf(out, "  build <source.cal> [-o output]   Build a native executable\n");
    fprintf(out, "  run <source.cal> [args...]       Build a temp executable and run it\n");
    fprintf(out, "  asm <source.cal>                 Emit x86_64 assembly to stdout\n");
    fprintf(out, "  bytecode <source.cal>            Emit portable bytecode text to stdout\n");
    fprintf(out, "  help                             Show this help\n");
}

static int emit_program_file(const char *path, CalyndaEmitMode mode) {
    int exit_code;

    if (mode == CALYNDA_EMIT_MODE_ASM) {
        MachineProgram machine_program;

        exit_code = calynda_compile_to_machine_program(path, &machine_program);
        if (exit_code != 0) {
            return exit_code;
        }
        if (!asm_emit_program(stdout, &machine_program)) {
            fprintf(stderr, "%s: failed to write assembly output\n", path);
            exit_code = 1;
        }
        machine_program_free(&machine_program);
        return exit_code;
    }

    {
        BytecodeProgram bytecode_program;

        exit_code = calynda_compile_to_bytecode_program(path, &bytecode_program);
        if (exit_code != 0) {
            return exit_code;
        }
        if (!bytecode_dump_program(stdout, &bytecode_program)) {
            fprintf(stderr, "%s: failed to write bytecode output\n", path);
            exit_code = 1;
        }
        bytecode_program_free(&bytecode_program);
    }
    return exit_code;
}

static int build_program_file(const char *source_path, const char *output_path) {
    MachineProgram machine_program;
    char *assembly;
    char assembly_path[PATH_MAX];
    char executable_dir[PATH_MAX];
    char runtime_object_path[PATH_MAX];
    int link_exit_code;
    int exit_code;

    exit_code = calynda_compile_to_machine_program(source_path, &machine_program);
    if (exit_code != 0) {
        return exit_code;
    }

    assembly = asm_emit_program_to_string(&machine_program);
    machine_program_free(&machine_program);
    if (!assembly) {
        fprintf(stderr, "%s: failed to render native assembly\n", source_path);
        return 1;
    }
    if (!calynda_write_temp_file("calynda-native", assembly, assembly_path, sizeof(assembly_path))) {
        fprintf(stderr, "%s: failed to create temporary assembly file\n", source_path);
        free(assembly);
        return 1;
    }
    if (!calynda_executable_directory(executable_dir, sizeof(executable_dir))) {
        strcpy(executable_dir, "build");
    }
    if (snprintf(runtime_object_path,
                 sizeof(runtime_object_path),
                 "%s/calynda_runtime.a",
                 executable_dir) < 0) {
        fprintf(stderr, "%s: failed to resolve runtime object path\n", source_path);
        unlink(assembly_path);
        free(assembly);
        return 1;
    }

    link_exit_code = calynda_run_linker(assembly_path, runtime_object_path, output_path);
    if (link_exit_code != 0) {
        fprintf(stderr,
                "%s: native link failed (gcc exit %d). Preserved assembly at %s\n",
                source_path,
                link_exit_code,
                assembly_path);
        free(assembly);
        return 1;
    }

    unlink(assembly_path);
    free(assembly);
    return 0;
}

static int run_program_file(const char *source_path, int argc, char **argv) {
    char executable_path[PATH_MAX];
    char template_path[] = "/tmp/calynda-run-XXXXXX";
    char **child_argv;
    int fd;
    int exit_code;
    int i;

    fd = mkstemp(template_path);
    if (fd < 0) {
        fprintf(stderr, "failed to create temporary executable path\n");
        return 1;
    }
    close(fd);
    unlink(template_path);
    memcpy(executable_path, template_path, sizeof(template_path));

    exit_code = build_program_file(source_path, executable_path);
    if (exit_code != 0) {
        unlink(executable_path);
        return exit_code;
    }

    child_argv = calloc((size_t)argc + 2, sizeof(*child_argv));
    if (!child_argv) {
        unlink(executable_path);
        fprintf(stderr, "out of memory while preparing process arguments\n");
        return 1;
    }
    child_argv[0] = executable_path;
    for (i = 0; i < argc; i++) {
        child_argv[i + 1] = argv[i];
    }
    child_argv[argc + 1] = NULL;

    exit_code = calynda_run_child_process(executable_path, child_argv);
    free(child_argv);
    unlink(executable_path);
    if (exit_code < 0) {
        fprintf(stderr, "%s: failed to run generated executable\n", source_path);
        return 1;
    }
    return exit_code;
}