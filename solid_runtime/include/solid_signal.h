#ifndef SOLID_SIGNAL_H
#define SOLID_SIGNAL_H

/*
 * solid_signal.h — Fine-grained reactive signals for Solid/Wii.
 *
 * Implements Solid.js-style createSignal():
 *   - Getter auto-tracks the calling effect via a global observer stack
 *   - Setter notifies all subscribed effects for re-execution
 *
 * All values are CalyndaRtWord (pointer-width) so they can carry ints,
 * bools, floats (bit-cast), or heap-allocated objects (strings, arrays).
 */

#include "calynda_runtime.h"
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---- Forward declarations ---- */
typedef struct SolidEffect SolidEffect;

/* ---- Signal ---- */

/* Maximum subscribers per signal before reallocation. */
#define SOLID_SIGNAL_INITIAL_SUBS 4

typedef struct SolidSignal {
    CalyndaRtWord   value;
    SolidEffect   **subscribers;
    size_t          subscriber_count;
    size_t          subscriber_cap;
} SolidSignal;

/*
 * Create a new signal with the given initial value.
 * Returns NULL on allocation failure.
 */
SolidSignal *solid_create_signal(CalyndaRtWord initial_value);

/*
 * Read the signal's current value.
 * If called inside an effect, the effect is automatically subscribed.
 */
CalyndaRtWord solid_signal_get(SolidSignal *sig);

/*
 * Write a new value to the signal.
 * If the value differs from the current one, all subscribed effects
 * are scheduled for re-execution (or run immediately if not batching).
 */
void solid_signal_set(SolidSignal *sig, CalyndaRtWord value);

/*
 * Free the signal and its subscriber array.
 * Does not dispose subscribed effects.
 */
void solid_signal_dispose(SolidSignal *sig);

/* ---- Observer tracking ---- */

/*
 * Push/pop an effect onto the global observer stack.
 * Used internally by the effect system; not for external use.
 */
void solid_observer_push(SolidEffect *effect);
void solid_observer_pop(void);

/*
 * Return the current observer (top of stack), or NULL if none.
 */
SolidEffect *solid_observer_current(void);

#ifdef __cplusplus
}
#endif

#endif /* SOLID_SIGNAL_H */
