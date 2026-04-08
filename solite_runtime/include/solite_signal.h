#ifndef SOLITE_SIGNAL_H
#define SOLITE_SIGNAL_H

/*
 * solite_signal.h — Fine-grained reactive signals for Solite/Wii.
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
typedef struct SoliteEffect SoliteEffect;

/* ---- Signal ---- */

/* Maximum subscribers per signal before reallocation. */
#define SOLID_SIGNAL_INITIAL_SUBS 4

typedef struct SoliteSignal {
    CalyndaRtWord   value;
    SoliteEffect   **subscribers;
    size_t          subscriber_count;
    size_t          subscriber_cap;
} SoliteSignal;

/*
 * Create a new signal with the given initial value.
 * Returns NULL on allocation failure.
 */
SoliteSignal *solite_create_signal(CalyndaRtWord initial_value);

/*
 * Read the signal's current value.
 * If called inside an effect, the effect is automatically subscribed.
 */
CalyndaRtWord solite_signal_get(SoliteSignal *sig);

/*
 * Write a new value to the signal.
 * If the value differs from the current one, all subscribed effects
 * are scheduled for re-execution (or run immediately if not batching).
 */
void solite_signal_set(SoliteSignal *sig, CalyndaRtWord value);

/*
 * Free the signal and its subscriber array.
 * Does not dispose subscribed effects.
 */
void solite_signal_dispose(SoliteSignal *sig);

/* ---- Observer tracking ---- */

/*
 * Push/pop an effect onto the global observer stack.
 * Used internally by the effect system; not for external use.
 */
void solid_observer_push(SoliteEffect *effect);
void solid_observer_pop(void);

/*
 * Return the current observer (top of stack), or NULL if none.
 */
SoliteEffect *solid_observer_current(void);

#ifdef __cplusplus
}
#endif

#endif /* SOLITE_SIGNAL_H */
