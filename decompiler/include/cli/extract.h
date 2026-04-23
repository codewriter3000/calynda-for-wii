#ifndef CALYNDA_DECOMPILER_CLI_EXTRACT_H
#define CALYNDA_DECOMPILER_CLI_EXTRACT_H

#include "dol.h"

/* Write every DOL section to <dir>/<kind><index>_0x<vaddr>.bin. */
int extract_sections(const DolImage *img, const char *dir);

#endif /* CALYNDA_DECOMPILER_CLI_EXTRACT_H */
