#include "cli/extract.h"

#include <stdio.h>
#include <stdlib.h>

static void ensure_dir(const char *dir) {
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "mkdir -p \"%s\"", dir);
    (void)system(cmd);
}

int extract_sections(const DolImage *img, const char *dir) {
    ensure_dir(dir);
    fprintf(stderr, "extracting %zu sections to: %s/\n",
            img->section_count, dir);
    for (size_t i = 0; i < img->section_count; i++) {
        const DolSection *s = &img->sections[i];
        char path[512];
        snprintf(path, sizeof(path), "%s/%s%u_0x%08x.bin",
                 dir,
                 s->kind == DOL_SECTION_TEXT ? "text" : "data",
                 s->index, s->virtual_addr);
        FILE *fp = fopen(path, "wb");
        if (!fp) { perror(path); return 1; }
        const uint8_t *data = img->bytes + s->file_offset;
        size_t written = fwrite(data, 1, s->size, fp);
        fclose(fp);
        if (written != (size_t)s->size) {
            fprintf(stderr, "error: wrote %zu/%u bytes to %s\n",
                    written, s->size, path);
            return 1;
        }
        fprintf(stderr, "  %s%u  0x%08x  %u bytes -> %s\n",
                s->kind == DOL_SECTION_TEXT ? "text" : "data",
                s->index, s->virtual_addr, s->size, path);
    }
    return 0;
}
