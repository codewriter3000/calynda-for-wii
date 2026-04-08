/*
 * solid_effect.c — Reactive effect and memo system for Solite/Wii.
 *
 * Effects auto-track signal dependencies by pushing themselves onto
 * the observer stack before running. Pending effects are queued and
 * flushed either immediately or at batch-end.
 */

#include "solite_effect.h"
#include "solite_signal.h"
#include "calynda_gc.h"

#include <string.h>

/* ---- Pending effect queue ---- */

#define SOLID_PENDING_MAX 256

static SoliteEffect *pending_queue[SOLID_PENDING_MAX];
static size_t       pending_count = 0;
static int          batch_depth = 0;

/* ---- Batching ---- */

void solite_batch_start(void)
{
    batch_depth++;
}

void solite_batch_end(void)
{
    if (batch_depth > 0) batch_depth--;
    if (batch_depth == 0) {
        solite_effect_flush_pending();
    }
}

bool solite_is_batching(void)
{
    return batch_depth > 0;
}

/* ---- Effect scheduling ---- */

void solite_effect_schedule(SoliteEffect *effect)
{
    if (!effect || effect->disposed || effect->pending) return;

    if (pending_count < SOLID_PENDING_MAX) {
        effect->pending = true;
        pending_queue[pending_count++] = effect;
    }
}

/*
 * Run a single effect: push on observer stack, execute, pop.
 */
static void effect_run(SoliteEffect *effect)
{
    if (!effect || effect->disposed) return;

    solid_observer_push(effect);
    effect->fn(effect->context);
    solid_observer_pop();
}

void solite_effect_flush_pending(void)
{
    if (batch_depth > 0) return;

    /* Process in queue order. Effects may schedule new effects
       during execution, so we drain iteratively. */
    while (pending_count > 0) {
        /* Snapshot the current queue */
        size_t count = pending_count;
        SoliteEffect *snapshot[SOLID_PENDING_MAX];
        memcpy(snapshot, pending_queue, sizeof(SoliteEffect *) * count);
        pending_count = 0;

        for (size_t i = 0; i < count; i++) {
            SoliteEffect *eff = snapshot[i];
            eff->pending = false;
            if (!eff->disposed) {
                effect_run(eff);
            }
        }
    }
}

/* ---- Effect creation ---- */

SoliteEffect *solite_create_effect(SoliteEffectFn fn, void *context)
{
    SoliteEffect *effect = (SoliteEffect *)calynda_gc_alloc(sizeof(SoliteEffect));
    if (!effect) return NULL;

    effect->fn = fn;
    effect->context = context;
    effect->pending = false;
    effect->disposed = false;

    /* Run immediately to establish initial subscriptions */
    effect_run(effect);

    return effect;
}

void solite_effect_dispose(SoliteEffect *effect)
{
    if (!effect) return;
    effect->disposed = true;
    /* Note: doesn't remove from signal subscriber lists. Disposed effects
       are simply skipped when encountered during notification. A full
       cleanup pass could be added if memory pressure requires it. */
}

/* ---- Memo ---- */

/*
 * Inner effect callback that marks the memo dirty when dependencies change.
 */
static void memo_recompute_cb(void *ctx)
{
    SoliteMemo *memo = (SoliteMemo *)ctx;
    if (!memo) return;
    memo->cached_value = memo->fn(memo->context);
    memo->dirty = false;
}

SoliteMemo *solid_create_memo(SoliteMemoFn fn, void *context)
{
    SoliteMemo *memo = (SoliteMemo *)calynda_gc_alloc(sizeof(SoliteMemo));
    if (!memo) return NULL;

    memo->fn = fn;
    memo->context = context;
    memo->dirty = true;
    memo->cached_value = (CalyndaRtWord)0;

    /* Create a tracking effect that recomputes the memo */
    memo->tracking_effect = solite_create_effect(memo_recompute_cb, memo);

    return memo;
}

CalyndaRtWord solite_memo_get(SoliteMemo *memo)
{
    if (!memo) return (CalyndaRtWord)0;

    /* Auto-track: if we're inside an effect, subscribe via the
       inner tracking effect's signal subscriptions (inherited) */
    return memo->cached_value;
}

void solite_memo_dispose(SoliteMemo *memo)
{
    if (!memo) return;
    if (memo->tracking_effect) {
        solite_effect_dispose(memo->tracking_effect);
    }
    calynda_gc_release(memo);
}
