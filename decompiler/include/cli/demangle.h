/*
 * CodeWarrior name demangler (requirement 5).
 *
 *   5.1 CodeWarrior mangler rule set — encode/decode Q<n>, F, f, v, ...
 *   5.2 Demangling library — `cw_demangle` produces a human-readable
 *       "Namespace::Class::method(argtypes)" string.
 *   5.3 Namespace -> Calynda package mapping — rewrite EGG::, nw4r::,
 *       JSystem:: qualifiers as egg, nw4r, jsystem package paths.
 *   5.4 Constructor / destructor identification — __ct / __dt are
 *       surfaced via `CwDemangled.kind`.
 *   5.5 Operator overload mapping — __ml / __pl / __eq / ... map to
 *       the corresponding Calynda operator syntax via `cw_operator`.
 *   5.6 Uniqueness enforcement — `CwRenameTable` detects collisions in
 *       the Calynda form and appends a numeric suffix.
 *
 * This module is pure: no DOL / SymTable dependency.  It operates on
 * raw mangled strings produced by Metrowerks CodeWarrior.
 */
#ifndef CLI_DEMANGLE_H
#define CLI_DEMANGLE_H

#include <stddef.h>

typedef enum {
    CW_KIND_REGULAR = 0,
    CW_KIND_CTOR    = 1,  /* __ct                                    */
    CW_KIND_DTOR    = 2,  /* __dt                                    */
    CW_KIND_OPERATOR = 3, /* __pl, __ml, __eq, ...                   */
    CW_KIND_FREE    = 4,  /* no class qualifier (free function)      */
} CwSymbolKind;

typedef struct {
    int ok;                   /* 1 on success, 0 if not mangled      */
    CwSymbolKind kind;
    char qualifier[256];      /* "nw4r::ut::DvdFileStream" or ""     */
    char method[128];         /* "Read", "operator+", "~DvdFileStream" */
    char args[512];           /* "(void)", "(int, const char*)"      */
    char pretty[1024];        /* qualifier::method args              */
} CwDemangled;

/* Parse a CodeWarrior-mangled symbol.  Always initialises `out`.
 * Returns 1 if parsing succeeded, 0 if the name was not recognised as
 * mangled (pretty == mangled verbatim in that case). */
int cw_demangle(const char *mangled, CwDemangled *out);

/* Map a C++ operator mnemonic (e.g. "pl", "ml", "eq") to its
 * human-readable spelling ("+", "*", "==").  Returns NULL if unknown.*/
const char *cw_operator(const char *mnemonic);

/* Rewrite a qualifier like "EGG::Heap" into a Calynda package path
 * ("egg.Heap") using the fixed prefix table (EGG, nw4r, JSystem, ...).
 * Writes at most `n` bytes to `out` (always NUL-terminated). */
void cw_to_calynda_path(const char *qualifier, char *out, size_t n);

/* Classify the Calynda binding style for a demangled symbol:
 *   CTOR -> factory (".new")
 *   DTOR -> drop    (".drop")
 *   OP   -> operator glyph
 *   else -> method name unchanged
 * Writes a full Calynda-style identifier ("egg.Heap.new") to `out`. */
void cw_to_calynda_binding(const CwDemangled *d, char *out, size_t n);

/* --- 5.6 uniqueness enforcement ------------------------------------ */

typedef struct CwRenameTable CwRenameTable;

CwRenameTable *cw_rename_new(void);
void cw_rename_free(CwRenameTable *t);

/* Return a name that is unique within the table.  The first time a
 * given base name is inserted it is returned unchanged; later inserts
 * get a numeric suffix ("_2", "_3", ...).  Result is copied into
 * `out`.  Returns 1 if the name required disambiguation. */
int cw_rename_unique(CwRenameTable *t, const char *base,
                     char *out, size_t n);

#endif
