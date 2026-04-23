/*
 * File handle with line counting + 250-line rotation (req 8.6).
 *
 * Paths follow:
 *   <dir>/<basename>.cal           — first part
 *   <dir>/<basename>_part2.cal     — spill once the primary crosses the
 *                                    configured line limit, and so on.
 */
#include "emit_cal_internal.h"

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

static int ensure_dir(const char *dir) {
    struct stat st;
    if (stat(dir, &st) == 0) return S_ISDIR(st.st_mode) ? 0 : -1;
    if (errno != ENOENT) return -1;
    return mkdir(dir, 0755);
}

static int open_part(EmitFile *ef) {
    char path[512];
    if (ef->part <= 1)
        snprintf(path, sizeof path, "%s/%s.cal", ef->dir, ef->basename);
    else
        snprintf(path, sizeof path, "%s/%s_part%u.cal",
                 ef->dir, ef->basename, ef->part);
    ef->fp = fopen(path, "w");
    if (!ef->fp) return -1;
    ef->lines = 0;
    ef->total_files++;
    return 0;
}

int emit_file_open(EmitFile *ef, const char *dir, const char *basename,
                   unsigned limit) {
    memset(ef, 0, sizeof *ef);
    if (ensure_dir(dir) != 0) return -1;
    ef->dir   = dir;
    ef->limit = limit ? limit : EMIT_CAL_LINE_LIMIT;
    ef->part  = 1;
    snprintf(ef->basename, sizeof ef->basename, "%s", basename);
    return open_part(ef);
}

void emit_file_close(EmitFile *ef) {
    if (ef->fp) { fclose(ef->fp); ef->fp = NULL; }
}

/* Rotate to the next sibling file. Emits a closing + reopening
 * comment so the split is self-describing. */
static int rotate(EmitFile *ef) {
    if (!ef->fp) return -1;
    fprintf(ef->fp, "# --- continues in %s_part%u.cal ---\n",
            ef->basename, ef->part + 1);
    fclose(ef->fp);
    ef->fp = NULL;
    ef->part++;
    ef->split_siblings++;
    if (open_part(ef) != 0) return -1;
    fprintf(ef->fp, "# --- continued from %s_part%u.cal ---\n",
            ef->basename, ef->part - 1);
    ef->lines = 1;
    return 0;
}

int emit_blank(EmitFile *ef) { return emit_linef(ef, ""); }

int emit_linef(EmitFile *ef, const char *fmt, ...) {
    if (!ef->fp) return -1;

    /* Count how many newlines this write adds so we can rotate
     * between lines rather than mid-statement. */
    va_list ap;
    char    buf[1024];
    va_start(ap, fmt);
    int n = vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    if (n < 0) return -1;

    unsigned nl = 1;          /* emit_linef always appends '\n' */
    for (int i = 0; i < n; i++) if (buf[i] == '\n') nl++;

    if (ef->lines + nl > ef->limit && ef->lines > 4 /* keep header together */)
        if (rotate(ef) != 0) return -1;

    fputs(buf, ef->fp);
    fputc('\n', ef->fp);
    ef->lines += nl;
    return 0;
}
