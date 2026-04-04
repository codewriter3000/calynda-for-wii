#include "calynda_gc.h"

#include <stdlib.h>
#include <string.h>

/*
 * Reference-counting GC.  Each allocation is preceded by a hidden size_t
 * refcount field; the pointer returned to callers points just past it.
 */

static size_t *header_of(void *ptr) {
    return (size_t *)ptr - 1;
}

void *calynda_gc_alloc(size_t size) {
    size_t *block = calloc(1, sizeof(size_t) + size);

    if (!block) {
        return NULL;
    }
    *block = 1; /* initial refcount */
    return block + 1;
}

void calynda_gc_retain(void *ptr) {
    if (!ptr) {
        return;
    }
    (*header_of(ptr))++;
}

void calynda_gc_release(void *ptr) {
    size_t *hdr;

    if (!ptr) {
        return;
    }
    hdr = header_of(ptr);
    if (*hdr == 0) {
        return;
    }
    (*hdr)--;
    if (*hdr == 0) {
        free(hdr);
    }
}

void calynda_gc_shutdown(void) {
    /* No-op: process exit reclaims all memory. */
}
