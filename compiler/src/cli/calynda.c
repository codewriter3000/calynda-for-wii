#define _GNU_SOURCE

#include "calynda_internal.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static bool parse_build_args(int argc, char **argv,
                             const char *default_output,
                             const char **output_path,
                             const char **target);
static void print_usage(FILE *out, const char *program_name);
static int emit_c_program_file(const char *path);
static int build_program_file(const char *source_path,
                              const char *output_path,
                              const char *target);
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
    if (strcmp(argv[1], "emit-c") == 0) {
        if (argc != 3) {
            print_usage(stderr, program_name);
            return 64;
        }
        return emit_c_program_file(argv[2]);
    }
    if (strcmp(argv[1], "build") == 0) {
        const char *output_path;
        const char *target;
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
        if (!parse_build_args(argc - 3, argv + 3, default_output, &output_path, &target)) {
            print_usage(stderr, program_name);
            return 64;
        }
        return build_program_file(source_path, output_path, target);
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

static bool parse_build_args(int argc, char **argv,
                             const char *default_output,
                             const char **output_path,
                             const char **target) {
    int i;

    if (!output_path || !default_output || !target) {
        return false;
    }

    *output_path = default_output;
    *target = "host";

    for (i = 0; i < argc; i++) {
        if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            *output_path = argv[i + 1];
            i++;
        } else if (strcmp(argv[i], "--target") == 0 && i + 1 < argc) {
            *target = argv[i + 1];
            i++;
        } else {
            return false;
        }
    }
    return true;
}

static void print_usage(FILE *out, const char *program_name) {
    fprintf(out, "Usage: %s <command> ...\n", program_name);
    fprintf(out, "\nCommands:\n");
    fprintf(out, "  build <source.cal> [-o output] [--target host|wii|gc]   Build an executable via C\n");
    fprintf(out, "  run <source.cal> [args...]                               Build a temp executable and run it\n");
    fprintf(out, "  emit-c <source.cal>                                      Emit generated C to stdout\n");
    fprintf(out, "  help                                                     Show this help\n");
}

static int emit_c_program_file(const char *path) {
    return calynda_compile_to_c(path, stdout);
}

static int build_program_file(const char *source_path,
                              const char *output_path,
                              const char *target) {
    char c_path[PATH_MAX];
    char include_dir[PATH_MAX];
    char lib_dir[PATH_MAX];
    int exit_code;
    FILE *c_file;

    /* Write generated C to a temp file */
    {
        char template_path[] = "/tmp/calynda-c-XXXXXX.c";
        int fd;
#ifdef __linux__
        fd = mkstemps(template_path, 2);
#else
        fd = mkstemp(template_path);
#endif
        if (fd < 0) {
            fprintf(stderr, "%s: failed to create temp C file\n", source_path);
            return 1;
        }
        c_file = fdopen(fd, "w");
        if (!c_file) {
            close(fd);
            fprintf(stderr, "%s: failed to open temp C file\n", source_path);
            return 1;
        }
        memcpy(c_path, template_path, sizeof(template_path));
    }

    exit_code = calynda_compile_to_c(source_path, c_file);
    fclose(c_file);
    if (exit_code != 0) {
        unlink(c_path);
        return exit_code;
    }

    /* Resolve runtime paths relative to the calynda executable */
    if (!calynda_executable_directory(include_dir, sizeof(include_dir))) {
        strcpy(include_dir, "build");
    }
    memcpy(lib_dir, include_dir, strlen(include_dir) + 1);

    exit_code = calynda_run_c_compiler(c_path, include_dir, lib_dir, output_path, target);
    if (exit_code != 0) {
        fprintf(stderr,
                "%s: C compilation failed (exit %d). Preserved C source at %s\n",
                source_path, exit_code, c_path);
        return 1;
    }

    unlink(c_path);
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

    exit_code = build_program_file(source_path, executable_path, "host");
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
