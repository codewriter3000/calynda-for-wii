/*
 * solid_signal.c — Fine-grained reactive signals for Solite/Wii.
 *
 * Implements auto-tracking via a global observer stack:
 *   - solite_signal_get() records the current effect as a subscriber
 *   - solite_signal_set() notifies all subscribers when value changes
 */

#include "solite_signal.h"
#include "solite_effect.h"
#include "calynda_gc.h"

#include <stdlib.h>
#include <string.h>

/* ---- Global observer stack ---- */

#define SOLID_OBSERVER_STACK_MAX 64

static SoliteEffect *observer_stack[SOLID_OBSERVER_STACK_MAX];
static size_t       observer_depth = 0;

void solid_observer_push(SoliteEffect *effect)
{
    if (observer_depth < SOLID_OBSERVER_STACK_MAX)
        observer_stack[observer_depth++] = effect;
}

void solid_observer_pop(void)
{
    if (observer_depth > 0)
        --observer_depth;
}

SoliteEffect *solid_observer_current(void)
{
    return observer_depth > 0 ? observer_stack[observer_depth - 1] : NULL;
}

/* ---- Signal implementation ---- */

SoliteSignal *solite_create_signal(CalyndaRtWord initial_value)
{
    SoliteSignal *sig = (SoliteSignal *)calynda_gc_alloc(sizeof(SoliteSignal));
    if (!sig) return NULL;

    sig->value = initial_value;
    sig->subscriber_count = 0;
    sig->subscriber_cap = SOLID_SIGNAL_INITIAL_SUBS;
    sig->subscribers = (SoliteEffect **)calynda_gc_alloc(
        sizeof(SoliteEffect *) * SOLID_SIGNAL_INITIAL_SUBS);
    if (!sig->subscribers) {
        calynda_gc_release(sig);
        return NULL;
    }
    return sig;
}

/*
 * Subscribe an effect to this signal (if not already subscribed).
 */
static void signal_subscribe(SoliteSignal *sig, SoliteEffect *effect)
{
    if (!sig || !effect) return;

    /* Check for duplicate */
    for (size_t i = 0; i < sig->subscriber_count; i++) {
        if (sig->subscribers[i] == effect) return;
    }

    /* Grow if needed */
    if (sig->subscriber_count >= sig->subscriber_cap) {
        size_t new_cap = sig->subscriber_cap * 2;
        SoliteEffect **new_subs = (SoliteEffect **)calynda_gc_alloc(
            sizeof(SoliteEffect *) * new_cap);
        if (!new_subs) return;
        memcpy(new_subs, sig->subscribers, sizeof(SoliteEffect *) * sig->subscriber_count);
        calynda_gc_release(sig->subscribers);
        sig->subscribers = new_subs;
        sig->subscriber_cap = new_cap;
    }

    sig->subscribers[sig->subscriber_count++] = effect;
}

CalyndaRtWord solite_signal_get(SoliteSignal *sig)
{
    if (!sig) return (CalyndaRtWord)0;

    /* Auto-track: if we're inside an effect, subscribe it */
    SoliteEffect *current = solid_observer_current();
    if (current) {
        signal_subscribe(sig, current);
    }

    return sig->value;
}

void solite_signal_set(SoliteSignal *sig, CalyndaRtWord value)
{
    if (!sig) return;
    if (sig->value == value) return; /* no change, skip notifications */

    sig->value = value;

    /* Notify all subscribed effects */
    for (size_t i = 0; i < sig->subscriber_count; i++) {
        solite_effect_schedule(sig->subscribers[i]);
    }

    /* Flush if not batching */
    solite_effect_flush_pending();
}

void solite_signal_dispose(SoliteSignal *sig)
{
    if (!sig) return;
    if (sig->subscribers) {
        calynda_gc_release(sig->subscribers);
    }
    calynda_gc_release(sig);
}
