/*
 * test_solite_effect.c — Unit tests for the effect and memo system.
 *
 * Tests:
 *   1. Effect runs immediately on creation
 *   2. Effect re-runs on dependency change
 *   3. Batching defers effect execution
 *   4. Memo caches computed value
 *   5. Memo recomputes when dependency changes
 *   6. Nested effects
 */

#include "solite_signal.h"
#include "solite_effect.h"

#include <assert.h>
#include <stdio.h>

/* ---- Test state ---- */
static int run_count = 0;
static CalyndaRtWord observed = 0;

static void counting_effect(void *ctx)
{
    SoliteSignal *sig = (SoliteSignal *)ctx;
    observed = solite_signal_get(sig);
    run_count++;
}

/* ---- Tests ---- */

static void test_effect_immediate_run(void)
{
    SoliteSignal *sig = solite_create_signal((CalyndaRtWord)10);
    run_count = 0;

    SoliteEffect *eff = solite_create_effect(counting_effect, sig);
    assert(run_count == 1);
    assert(observed == (CalyndaRtWord)10);

    solite_effect_dispose(eff);
    solite_signal_dispose(sig);
    printf("  [PASS] test_effect_immediate_run\n");
}

static void test_effect_rerun(void)
{
    SoliteSignal *sig = solite_create_signal((CalyndaRtWord)0);
    run_count = 0;

    SoliteEffect *eff = solite_create_effect(counting_effect, sig);
    assert(run_count == 1);

    solite_signal_set(sig, (CalyndaRtWord)42);
    assert(run_count == 2);
    assert(observed == (CalyndaRtWord)42);

    solite_effect_dispose(eff);
    solite_signal_dispose(sig);
    printf("  [PASS] test_effect_rerun\n");
}

static void test_batching(void)
{
    SoliteSignal *sig = solite_create_signal((CalyndaRtWord)0);
    run_count = 0;

    SoliteEffect *eff = solite_create_effect(counting_effect, sig);
    assert(run_count == 1);

    solite_batch_start();
    solite_signal_set(sig, (CalyndaRtWord)1);
    solite_signal_set(sig, (CalyndaRtWord)2);
    solite_signal_set(sig, (CalyndaRtWord)3);

    /* During batch, effect should not have re-run yet */
    assert(run_count == 1);

    solite_batch_end();

    /* After batch end, effect should have run (effect was scheduled
       on the first set that changed value; subsequent sets schedule
       again but the effect pending flag prevents duplicates) */
    assert(run_count == 2);
    assert(observed == (CalyndaRtWord)3);

    solite_effect_dispose(eff);
    solite_signal_dispose(sig);
    printf("  [PASS] test_batching\n");
}

/* Memo test state */
static SoliteSignal *memo_dep_sig = NULL;
static CalyndaRtWord memo_compute_fn(void *ctx)
{
    (void)ctx;
    return solite_signal_get(memo_dep_sig) * 2;
}

static void test_memo_caches(void)
{
    memo_dep_sig = solite_create_signal((CalyndaRtWord)5);

    SoliteMemo *memo = solid_create_memo(memo_compute_fn, NULL);
    assert(memo != NULL);
    assert(solite_memo_get(memo) == (CalyndaRtWord)10);
    /* A second get should return cached value */
    assert(solite_memo_get(memo) == (CalyndaRtWord)10);

    solite_memo_dispose(memo);
    solite_signal_dispose(memo_dep_sig);
    memo_dep_sig = NULL;
    printf("  [PASS] test_memo_caches\n");
}

static void test_memo_recomputes(void)
{
    memo_dep_sig = solite_create_signal((CalyndaRtWord)3);

    SoliteMemo *memo = solid_create_memo(memo_compute_fn, NULL);
    assert(solite_memo_get(memo) == (CalyndaRtWord)6);

    solite_signal_set(memo_dep_sig, (CalyndaRtWord)7);
    assert(solite_memo_get(memo) == (CalyndaRtWord)14);

    solite_memo_dispose(memo);
    solite_signal_dispose(memo_dep_sig);
    memo_dep_sig = NULL;
    printf("  [PASS] test_memo_recomputes\n");
}

/* Nested effects */
static int outer_count = 0;
static int inner_count = 0;

static void inner_effect(void *ctx)
{
    SoliteSignal *sig = (SoliteSignal *)ctx;
    (void)solite_signal_get(sig);
    inner_count++;
}

static SoliteSignal *nested_sig = NULL;
static void outer_effect(void *ctx)
{
    (void)ctx;
    (void)solite_signal_get(nested_sig);
    outer_count++;
}

static void test_nested_effects(void)
{
    nested_sig = solite_create_signal((CalyndaRtWord)1);
    outer_count = 0;
    inner_count = 0;

    SoliteEffect *outer = solite_create_effect(outer_effect, NULL);
    SoliteEffect *inner = solite_create_effect(inner_effect, nested_sig);
    assert(outer_count == 1);
    assert(inner_count == 1);

    solite_signal_set(nested_sig, (CalyndaRtWord)2);
    assert(outer_count == 2);
    assert(inner_count == 2);

    solite_effect_dispose(outer);
    solite_effect_dispose(inner);
    solite_signal_dispose(nested_sig);
    nested_sig = NULL;
    printf("  [PASS] test_nested_effects\n");
}

/* ---- Main ---- */

int main(void)
{
    printf("=== test_solite_effect ===\n");
    test_effect_immediate_run();
    test_effect_rerun();
    test_batching();
    test_memo_caches();
    test_memo_recomputes();
    test_nested_effects();
    printf("=== All effect tests passed ===\n");
    return 0;
}
