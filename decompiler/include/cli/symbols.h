/*
 * Symbol table loaded from an ogws-style symbols.txt file.
 * Each entry records a virtual address and optional size range.
 */
#ifndef CALYNDA_DECOMPILER_CLI_SYMBOLS_H
#define CALYNDA_DECOMPILER_CLI_SYMBOLS_H

#include <stddef.h>
#include <stdint.h>

#define SYM_NAME_MAX 128

typedef struct {
    uint32_t vaddr;
    uint32_t size;      /* 0 when unknown */
    char     name[SYM_NAME_MAX];
} SymEntry;

typedef struct {
    SymEntry *entries;
    size_t    count;
    size_t    cap;
} SymTable;

void        sym_table_init(SymTable *st);
void        sym_table_free(SymTable *st);
int         sym_table_load(SymTable *st, const char *path);

/* Binary-search lookup at exact vaddr; returns NULL if missing. */
const char       *sym_lookup(const SymTable *st, uint32_t vaddr);

/* Linear scan by name; returns NULL if missing. */
const SymEntry   *sym_find_by_name(const SymTable *st, const char *name);

/*
 * Return the symbol whose [vaddr, vaddr+size) covers `addr`, or NULL.
 * Entries with size == 0 are treated as 4 bytes (one instruction).
 */
const SymEntry   *sym_covering(const SymTable *st, uint32_t addr);

#endif /* CALYNDA_DECOMPILER_CLI_SYMBOLS_H */
