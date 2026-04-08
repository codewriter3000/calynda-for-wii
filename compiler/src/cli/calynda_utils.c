#define _POSIX_C_SOURCE 200809L

#include "calynda_internal.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

char *calynda_read_entire_file(const char *path) {
    FILE *file;
    char *buffer;
    long size;
    size_t read_size;

    file = fopen(path, "rb");
    if (!file) {
        fprintf(stderr, "%s: %s\n", path, strerror(errno));
        return NULL;
    }
    if (fseek(file, 0, SEEK_END) != 0) {
        fprintf(stderr, "%s: failed to seek: %s\n", path, strerror(errno));
        fclose(file);
        return NULL;
    }
    size = ftell(file);
    if (size < 0) {
        fprintf(stderr, "%s: failed to size file: %s\n", path, strerror(errno));
        fclose(file);
        return NULL;
    }
    if (fseek(file, 0, SEEK_SET) != 0) {
        fprintf(stderr, "%s: failed to rewind: %s\n", path, strerror(errno));
        fclose(file);
        return NULL;
    }

    buffer = malloc((size_t)size + 1);
    if (!buffer) {
        fprintf(stderr, "%s: out of memory while reading file\n", path);
        fclose(file);
        return NULL;
    }
    read_size = fread(buffer, 1, (size_t)size, file);
    fclose(file);
    if (read_size != (size_t)size) {
        fprintf(stderr, "%s: failed to read file contents\n", path);
        free(buffer);
        return NULL;
    }

    buffer[size] = '\0';
    return buffer;
}

bool calynda_executable_directory(char *buffer, size_t buffer_size) {
    ssize_t written;
    char *slash;

    if (!buffer || buffer_size == 0) {
        return false;
    }

    written = readlink("/proc/self/exe", buffer, buffer_size - 1);
    if (written < 0 || (size_t)written >= buffer_size) {
        return false;
    }
    buffer[written] = '\0';

    slash = strrchr(buffer, '/');
    if (!slash) {
        return false;
    }
    *slash = '\0';
    return true;
}

bool calynda_write_temp_file(const char *prefix,
                             const char *contents,
                             char *path_buffer,
                             size_t buffer_size) {
    char template_path[64];
    FILE *file;
    int fd;

    if (!prefix || !contents || !path_buffer || buffer_size == 0) {
        return false;
    }

    if (snprintf(template_path, sizeof(template_path), "/tmp/%s-XXXXXX", prefix) < 0) {
        return false;
    }
    fd = mkstemp(template_path);
    if (fd < 0) {
        return false;
    }

    file = fdopen(fd, "wb");
    if (!file) {
        close(fd);
        unlink(template_path);
        return false;
    }
    if (fputs(contents, file) == EOF || fclose(file) != 0) {
        unlink(template_path);
        return false;
    }
    if (strlen(template_path) + 1 > buffer_size) {
        unlink(template_path);
        return false;
    }

    memcpy(path_buffer, template_path, strlen(template_path) + 1);
    return true;
}

int calynda_run_c_compiler(const char *c_source_path,
                           const char *runtime_include_dir,
                           const char *runtime_lib_dir,
                           const char *output_path,
                           const char *target) {
    pid_t child;
    int status;
    char lib_flag[PATH_MAX + 3];
    char inc_flag[PATH_MAX + 3];
    char ogc_inc_flag[PATH_MAX + 32];
    char ogc_lib_flag[PATH_MAX + 32];
    const char *devkitpro;

    if (!c_source_path || !runtime_include_dir || !runtime_lib_dir ||
        !output_path || !target) {
        return -1;
    }

    snprintf(inc_flag, sizeof(inc_flag), "-I%s", runtime_include_dir);
    snprintf(lib_flag, sizeof(lib_flag), "-L%s", runtime_lib_dir);

    /* Resolve DEVKITPRO for libogc paths. */
    devkitpro = getenv("DEVKITPRO");
    if (!devkitpro) {
        devkitpro = "/opt/devkitpro";
    }
    snprintf(ogc_inc_flag, sizeof(ogc_inc_flag), "-I%s/libogc/include", devkitpro);
    snprintf(ogc_lib_flag, sizeof(ogc_lib_flag), "-L%s/libogc/lib/wii", devkitpro);

    child = fork();
    if (child < 0) {
        return -1;
    }

    if (child == 0) {
        if (strcmp(target, "wii") == 0 || strcmp(target, "gc") == 0) {
            char output_elf[PATH_MAX];
            char portlibs_flag[PATH_MAX + 32];
            char font_obj[PATH_MAX + 32];
            snprintf(output_elf, sizeof(output_elf), "%s.elf", output_path);
            snprintf(portlibs_flag, sizeof(portlibs_flag),
                     "-L%s/portlibs/ppc/lib", devkitpro);
            snprintf(font_obj, sizeof(font_obj),
                     "%s/font.ttf.o", runtime_lib_dir);
            const char *argv[] = {
                "powerpc-eabi-g++",
                "-DCALYNDA_WII_BUILD",
                "-x", "c",
                "-o", output_elf,
                c_source_path,
                inc_flag,
                ogc_inc_flag,
                lib_flag,
                ogc_lib_flag,
                portlibs_flag,
                "-lsolid_wii",
                "-lcalynda_runtime",
                "-lwiigui",
                "-lfreetype",
                "-lpng", "-lz",
                "-lbz2", "-lbrotlidec", "-lbrotlicommon",
                "-lvorbisidec", "-logg",
                "-lasnd",
                "-lwiiuse", "-lbte", "-logc",
                "-lm",
                "-mrvl", "-mcpu=750", "-meabi", "-mhard-float",
                NULL
            };
            execvp("powerpc-eabi-g++", (char *const *)argv);
        } else {
            execlp("gcc",
                   "gcc",
                   "-o", output_path,
                   c_source_path,
                   inc_flag,
                   "-lsolid_wii_host",
                   "-lcalynda_runtime",
                   lib_flag,
                   (char *)NULL);
        }
        _exit(127);
    }

    if (waitpid(child, &status, 0) < 0) {
        return -1;
    }
    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    }

    return -1;
}

int calynda_run_elf2dol(const char *elf_path, const char *dol_path) {
    pid_t child;
    int status;

    if (!elf_path || !dol_path) {
        return -1;
    }

    child = fork();
    if (child < 0) {
        return -1;
    }

    if (child == 0) {
        execlp("elf2dol",
               "elf2dol",
               elf_path,
               dol_path,
               (char *)NULL);
        _exit(127);
    }

    if (waitpid(child, &status, 0) < 0) {
        return -1;
    }
    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    }

    return -1;
}

int calynda_run_child_process(const char *path, char *const argv[]) {
    pid_t child;
    int status;

    child = fork();
    if (child < 0) {
        return -1;
    }

    if (child == 0) {
        execv(path, argv);
        _exit(127);
    }

    if (waitpid(child, &status, 0) < 0) {
        return -1;
    }
    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    }

    return -1;
}
