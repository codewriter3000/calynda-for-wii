#ifndef SOLITE_EFFECT_H
#define SOLITE_EFFECT_H

/*
 * solite_effect.h — Reactive effect system for Solite/Wii.
 *
 * Effects are callbacks that automatically re-run when their tracked
 * signals change. Memos are cached derivations that only recompute
 * when dependencies change.
 */

#include "calynda_runtime.h"
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---- Effect ---- */

typedef void (*SoliteEffectFn)(void *context);

typedef struct SoliteEffect {
    SoliteEffectFn   fn;
    void           *context;
    bool            pending;      /* scheduled for re-execution */
    bool            disposed;
} SoliteEffect;

/*
 * Create and immediately run an effect.
 * The effect re-runs whenever any signal read during its execution changes.
 */
SoliteEffect *solite_create_effect(SoliteEffectFn fn, void *context);

/*
 * Dispose an effect so it will no longer run.
 */
void solite_effect_dispose(SoliteEffect *effect);

/*
 * Schedule an effect for re-execution (called by signal_set).
 */
void solite_effect_schedule(SoliteEffect *effect);

/*
 * Flush all pending effects. During batching, this is deferred
 * until solite_batch_end(). Outside batching, called automatically.
 */
void solite_effect_flush_pending(void);

/* ---- Memo ---- */

typedef CalyndaRtWord (*SoliteMemoFn)(void *context);

typedef struct SoliteMemo {
    SoliteMemoFn     fn;
    void           *context;
    CalyndaRtWord   cached_value;
    SoliteEffect    *tracking_effect; /* inner effect that recomputes */
    bool            dirty;
} SoliteMemo;

/*
 * Create a memo — a cached derivation that recomputes when dependencies change.
 */
SoliteMemo *solid_create_memo(SoliteMemoFn fn, void *context);

/*
 * Read the memo value. Recomputes if dirty.
 */
CalyndaRtWord solite_memo_get(SoliteMemo *memo);

/*
 * Dispose a memo.
 */
void solite_memo_dispose(SoliteMemo *memo);

/* ---- Batching ---- */

/*
 * Begin a batch update. Signal changes during a batch are deferred
 * until solite_batch_end() is called.
 */
void solite_batch_start(void);

/*
 * End a batch update and flush all pending effects.
 */
void solite_batch_end(void);

/*
 * Returns true if we are currently inside a batch.
 */
bool solite_is_batching(void);

#ifdef __cplusplus
}
#endif

#endif /* SOLITE_EFFECT_H */
