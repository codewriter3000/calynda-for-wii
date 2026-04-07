#ifndef CALYNDA_GC_H
#define CALYNDA_GC_H

#include <stddef.h>

/* Allocate GC-managed memory (zero-initialised). Returns NULL on failure. */
void *calynda_gc_alloc(size_t size);

/* Retain an object (increment reference count). Safe to call with NULL. */
void  calynda_gc_retain(void *ptr);

/* Release an object (decrement reference count, free if zero). Safe with NULL. */
void  calynda_gc_release(void *ptr);

/* Shutdown the GC (release all remaining objects). */
void  calynda_gc_shutdown(void);

#endif /* CALYNDA_GC_H */
