#ifndef SOLID_EFFECT_H
#define SOLID_EFFECT_H

/*
 * solid_effect.h — Reactive effect system for Solid/Wii.
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

typedef void (*SolidEffectFn)(void *context);

typedef struct SolidEffect {
    SolidEffectFn   fn;
    void           *context;
    bool            pending;      /* scheduled for re-execution */
    bool            disposed;
} SolidEffect;

/*
 * Create and immediately run an effect.
 * The effect re-runs whenever any signal read during its execution changes.
 */
SolidEffect *solid_create_effect(SolidEffectFn fn, void *context);

/*
 * Dispose an effect so it will no longer run.
 */
void solid_effect_dispose(SolidEffect *effect);

/*
 * Schedule an effect for re-execution (called by signal_set).
 */
void solid_effect_schedule(SolidEffect *effect);

/*
 * Flush all pending effects. During batching, this is deferred
 * until solid_batch_end(). Outside batching, called automatically.
 */
void solid_effect_flush_pending(void);

/* ---- Memo ---- */

typedef CalyndaRtWord (*SolidMemoFn)(void *context);

typedef struct SolidMemo {
    SolidMemoFn     fn;
    void           *context;
    CalyndaRtWord   cached_value;
    SolidEffect    *tracking_effect; /* inner effect that recomputes */
    bool            dirty;
} SolidMemo;

/*
 * Create a memo — a cached derivation that recomputes when dependencies change.
 */
SolidMemo *solid_create_memo(SolidMemoFn fn, void *context);

/*
 * Read the memo value. Recomputes if dirty.
 */
CalyndaRtWord solid_memo_get(SolidMemo *memo);

/*
 * Dispose a memo.
 */
void solid_memo_dispose(SolidMemo *memo);

/* ---- Batching ---- */

/*
 * Begin a batch update. Signal changes during a batch are deferred
 * until solid_batch_end() is called.
 */
void solid_batch_start(void);

/*
 * End a batch update and flush all pending effects.
 */
void solid_batch_end(void);

/*
 * Returns true if we are currently inside a batch.
 */
bool solid_is_batching(void);

#ifdef __cplusplus
}
#endif

#endif /* SOLID_EFFECT_H */
