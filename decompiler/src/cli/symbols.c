#include "cli/symbols.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void sym_table_init(SymTable *st) {
    st->entries = NULL; st->count = 0; st->cap = 0;
}

void sym_table_free(SymTable *st) {
    free(st->entries); sym_table_init(st);
}

static int sym_entry_cmp(const void *a, const void *b) {
    const SymEntry *ea = (const SymEntry *)a;
    const SymEntry *eb = (const SymEntry *)b;
    if (ea->vaddr < eb->vaddr) return -1;
    if (ea->vaddr > eb->vaddr) return  1;
    return 0;
}

static int sym_table_add(SymTable *st, uint32_t vaddr, uint32_t size,
                         const char *name) {
    if (st->count == st->cap) {
        size_t new_cap = st->cap ? st->cap * 2 : 256;
        SymEntry *tmp = (SymEntry *)realloc(st->entries,
                                            new_cap * sizeof(SymEntry));
        if (!tmp) return 0;
        st->entries = tmp; st->cap = new_cap;
    }
    st->entries[st->count].vaddr = vaddr;
    st->entries[st->count].size  = size;
    strncpy(st->entries[st->count].name, name, SYM_NAME_MAX - 1);
    st->entries[st->count].name[SYM_NAME_MAX - 1] = '\0';
    st->count++;
    return 1;
}

static void sym_table_sort(SymTable *st) {
    if (st->count > 1)
        qsort(st->entries, st->count, sizeof(SymEntry), sym_entry_cmp);
}

const char *sym_lookup(const SymTable *st, uint32_t vaddr) {
    if (!st || !st->count) return NULL;
    size_t lo = 0, hi = st->count;
    while (lo < hi) {
        size_t mid = lo + (hi - lo) / 2;
        if (st->entries[mid].vaddr == vaddr) return st->entries[mid].name;
        if (st->entries[mid].vaddr < vaddr) lo = mid + 1;
        else hi = mid;
    }
    return NULL;
}

const SymEntry *sym_find_by_name(const SymTable *st, const char *name) {
    if (!st || !name) return NULL;
    for (size_t i = 0; i < st->count; i++) {
        if (strcmp(st->entries[i].name, name) == 0) return &st->entries[i];
    }
    return NULL;
}

const SymEntry *sym_covering(const SymTable *st, uint32_t addr) {
    if (!st || !st->count) return NULL;
    size_t lo = 0, hi = st->count;
    while (lo < hi) {
        size_t mid = lo + (hi - lo) / 2;
        if (st->entries[mid].vaddr <= addr) lo = mid + 1;
        else hi = mid;
    }
    if (lo == 0) return NULL;
    const SymEntry *e = &st->entries[lo - 1];
    uint32_t size = e->size ? e->size : 4u;
    if (addr < e->vaddr + size) return e;
    return NULL;
}

int sym_table_load(SymTable *st, const char *path) {
    FILE *fp = fopen(path, "r");
    if (!fp) { perror(path); return 0; }
    char line[512];
    while (fgets(line, sizeof(line), fp)) {
        char *eq = strstr(line, " = ");
        if (!eq) continue;
        size_t nlen = (size_t)(eq - line);
        if (nlen == 0 || nlen >= SYM_NAME_MAX) continue;
        char name[SYM_NAME_MAX];
        memcpy(name, line, nlen);
        name[nlen] = '\0';
        while (nlen > 0 && (name[nlen-1] == ' ' || name[nlen-1] == '\t'))
            name[--nlen] = '\0';
        char *colon = strstr(eq + 3, ":0x");
        if (!colon) continue;
        uint32_t vaddr = (uint32_t)strtoul(colon + 1, NULL, 16);
        if (vaddr == 0) continue;
        uint32_t size = 0;
        char *sz = strstr(colon, "size:0x");
        if (sz) size = (uint32_t)strtoul(sz + 5, NULL, 16);
        if (!sym_table_add(st, vaddr, size, name)) break;
    }
    fclose(fp);
    sym_table_sort(st);
    return 1;
}
