/*
 * solid_signal.c — Fine-grained reactive signals for Solid/Wii.
 *
 * Implements auto-tracking via a global observer stack:
 *   - solid_signal_get() records the current effect as a subscriber
 *   - solid_signal_set() notifies all subscribers when value changes
 */

#include "solid_signal.h"
#include "solid_effect.h"
#include "calynda_gc.h"

#include <stdlib.h>
#include <string.h>

/* ---- Global observer stack ---- */

#define SOLID_OBSERVER_STACK_MAX 64

static SolidEffect *observer_stack[SOLID_OBSERVER_STACK_MAX];
static size_t       observer_depth = 0;

void solid_observer_push(SolidEffect *effect)
{
    if (observer_depth < SOLID_OBSERVER_STACK_MAX)
        observer_stack[observer_depth++] = effect;
}

void solid_observer_pop(void)
{
    if (observer_depth > 0)
        --observer_depth;
}

SolidEffect *solid_observer_current(void)
{
    return observer_depth > 0 ? observer_stack[observer_depth - 1] : NULL;
}

/* ---- Signal implementation ---- */

SolidSignal *solid_create_signal(CalyndaRtWord initial_value)
{
    SolidSignal *sig = (SolidSignal *)calynda_gc_alloc(sizeof(SolidSignal));
    if (!sig) return NULL;

    sig->value = initial_value;
    sig->subscriber_count = 0;
    sig->subscriber_cap = SOLID_SIGNAL_INITIAL_SUBS;
    sig->subscribers = (SolidEffect **)calynda_gc_alloc(
        sizeof(SolidEffect *) * SOLID_SIGNAL_INITIAL_SUBS);
    if (!sig->subscribers) {
        calynda_gc_release(sig);
        return NULL;
    }
    return sig;
}

/*
 * Subscribe an effect to this signal (if not already subscribed).
 */
static void signal_subscribe(SolidSignal *sig, SolidEffect *effect)
{
    if (!sig || !effect) return;

    /* Check for duplicate */
    for (size_t i = 0; i < sig->subscriber_count; i++) {
        if (sig->subscribers[i] == effect) return;
    }

    /* Grow if needed */
    if (sig->subscriber_count >= sig->subscriber_cap) {
        size_t new_cap = sig->subscriber_cap * 2;
        SolidEffect **new_subs = (SolidEffect **)calynda_gc_alloc(
            sizeof(SolidEffect *) * new_cap);
        if (!new_subs) return;
        memcpy(new_subs, sig->subscribers, sizeof(SolidEffect *) * sig->subscriber_count);
        calynda_gc_release(sig->subscribers);
        sig->subscribers = new_subs;
        sig->subscriber_cap = new_cap;
    }

    sig->subscribers[sig->subscriber_count++] = effect;
}

CalyndaRtWord solid_signal_get(SolidSignal *sig)
{
    if (!sig) return (CalyndaRtWord)0;

    /* Auto-track: if we're inside an effect, subscribe it */
    SolidEffect *current = solid_observer_current();
    if (current) {
        signal_subscribe(sig, current);
    }

    return sig->value;
}

void solid_signal_set(SolidSignal *sig, CalyndaRtWord value)
{
    if (!sig) return;
    if (sig->value == value) return; /* no change, skip notifications */

    sig->value = value;

    /* Notify all subscribed effects */
    for (size_t i = 0; i < sig->subscriber_count; i++) {
        solid_effect_schedule(sig->subscribers[i]);
    }

    /* Flush if not batching */
    solid_effect_flush_pending();
}

void solid_signal_dispose(SolidSignal *sig)
{
    if (!sig) return;
    if (sig->subscribers) {
        calynda_gc_release(sig->subscribers);
    }
    calynda_gc_release(sig);
}
