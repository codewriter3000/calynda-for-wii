/*
 * RVL SDK extern binding generator (requirement 6).
 *
 *   6.1 Symbol inventory — walk every bl target plus every symbol
 *       and flag those matching an SDK prefix or pointing outside
 *       any mapped DOL text section.
 *   6.2 SDK library classification — bucket each external into
 *       `os`, `gx`, `pad`, `ax`, `nw4r`, `msl`, ... by prefix.
 *   6.3 Calynda type signature — convert the demangled C++ arg list
 *       to Calynda primitives (i32, u32, f32, *T, ...).
 *   6.4 Variadic / callback patterns — `...` tails and
 *       `ret(*)(args)` function-pointer arguments are carried
 *       through verbatim in Calynda form (`...args`, `fn(..)->R`).
 *   6.5 Header generation — emit one `.cal` per module under an
 *       output directory.
 *   6.6 Validation — a prefix table with known good argument counts
 *       emits a warning when a detected signature disagrees.
 */
#ifndef CLI_EXTERNS_H
#define CLI_EXTERNS_H

#include "dol.h"
#include "cli/symbols.h"

#include <stdint.h>
#include <stdio.h>

#define EX_MODULE_MAX 24

typedef struct {
    uint32_t vaddr;
    char     module[EX_MODULE_MAX];   /* "os", "gx", "", ...          */
    char     name[SYM_NAME_MAX];      /* Calynda-safe identifier      */
    char     raw_name[SYM_NAME_MAX];  /* original mangled symbol      */
    char     cal_sig[512];            /* "fn(a: i32, ...) -> u32"     */
    int      is_variadic;
    int      is_unmapped;             /* bl target outside text       */
    int      validated;               /* matched known reference      */
} ExternSymbol;

typedef struct {
    ExternSymbol *entries;
    size_t        count;
    size_t        cap;
} Externs;

/* Classify a raw symbol name into an SDK module (e.g. "OS" -> "os",
 * "GX..." -> "gx", "nw4r::..." -> "nw4r").  Returns an empty string
 * when no module matches. */
const char *externs_classify(const char *raw_name);

/* Convert a C++ type string (as emitted by the demangler) to a
 * Calynda type string.  Writes at most `n` bytes. */
void externs_cpp_type_to_calynda(const char *cpp, char *out, size_t n);

/* Build the full external inventory for a DOL + symbol table. */
int externs_build(const DolImage *img, const SymTable *syms, Externs *out);

/* Write one .cal header file per module to `dir`.  Directory must
 * exist.  Returns the number of files emitted. */
int externs_emit_dir(const Externs *ex, const char *dir);

void externs_print(const Externs *ex, FILE *out);

void externs_free(Externs *ex);

#endif
