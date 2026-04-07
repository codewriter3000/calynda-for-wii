/*
 * test_solid_signal.c — Unit tests for the reactive signal system.
 *
 * Tests:
 *   1. Signal creation and initial value
 *   2. Signal get/set
 *   3. Auto-tracking (effect subscribes on read)
 *   4. Notification (effect re-runs on set)
 *   5. No notification when value unchanged
 *   6. Multiple subscribers
 *   7. Signal dispose
 */

#include "solid_signal.h"
#include "solid_effect.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

/* ---- Test state ---- */
static int effect_run_count = 0;
static CalyndaRtWord last_observed_value = 0;

static void test_effect_fn(void *ctx)
{
    SolidSignal *sig = (SolidSignal *)ctx;
    last_observed_value = solid_signal_get(sig);
    effect_run_count++;
}

/* ---- Tests ---- */

static void test_create_and_get(void)
{
    SolidSignal *sig = solid_create_signal((CalyndaRtWord)42);
    assert(sig != NULL);
    assert(solid_signal_get(sig) == (CalyndaRtWord)42);
    solid_signal_dispose(sig);
    printf("  [PASS] test_create_and_get\n");
}

static void test_set_changes_value(void)
{
    SolidSignal *sig = solid_create_signal((CalyndaRtWord)10);
    solid_signal_set(sig, (CalyndaRtWord)20);
    assert(solid_signal_get(sig) == (CalyndaRtWord)20);
    solid_signal_dispose(sig);
    printf("  [PASS] test_set_changes_value\n");
}

static void test_auto_tracking(void)
{
    SolidSignal *sig = solid_create_signal((CalyndaRtWord)5);
    effect_run_count = 0;
    last_observed_value = 0;

    /* Creating an effect should run it immediately */
    SolidEffect *eff = solid_create_effect(test_effect_fn, sig);
    assert(eff != NULL);
    assert(effect_run_count == 1);
    assert(last_observed_value == (CalyndaRtWord)5);

    /* Signal should now have 1 subscriber */
    assert(sig->subscriber_count == 1);

    solid_effect_dispose(eff);
    solid_signal_dispose(sig);
    printf("  [PASS] test_auto_tracking\n");
}

static void test_notification(void)
{
    SolidSignal *sig = solid_create_signal((CalyndaRtWord)0);
    effect_run_count = 0;

    SolidEffect *eff = solid_create_effect(test_effect_fn, sig);
    assert(effect_run_count == 1); /* initial run */

    solid_signal_set(sig, (CalyndaRtWord)100);
    assert(effect_run_count == 2);
    assert(last_observed_value == (CalyndaRtWord)100);

    solid_signal_set(sig, (CalyndaRtWord)200);
    assert(effect_run_count == 3);
    assert(last_observed_value == (CalyndaRtWord)200);

    solid_effect_dispose(eff);
    solid_signal_dispose(sig);
    printf("  [PASS] test_notification\n");
}

static void test_no_notification_same_value(void)
{
    SolidSignal *sig = solid_create_signal((CalyndaRtWord)7);
    effect_run_count = 0;

    SolidEffect *eff = solid_create_effect(test_effect_fn, sig);
    assert(effect_run_count == 1);

    /* Set same value — should NOT re-run the effect */
    solid_signal_set(sig, (CalyndaRtWord)7);
    assert(effect_run_count == 1);

    solid_effect_dispose(eff);
    solid_signal_dispose(sig);
    printf("  [PASS] test_no_notification_same_value\n");
}

static int effect_a_count = 0;
static int effect_b_count = 0;

static void effect_a_fn(void *ctx) {
    solid_signal_get((SolidSignal *)ctx);
    effect_a_count++;
}
static void effect_b_fn(void *ctx) {
    solid_signal_get((SolidSignal *)ctx);
    effect_b_count++;
}

static void test_multiple_subscribers(void)
{
    SolidSignal *sig = solid_create_signal((CalyndaRtWord)0);
    effect_a_count = 0;
    effect_b_count = 0;

    SolidEffect *ea = solid_create_effect(effect_a_fn, sig);
    SolidEffect *eb = solid_create_effect(effect_b_fn, sig);
    assert(effect_a_count == 1);
    assert(effect_b_count == 1);
    assert(sig->subscriber_count == 2);

    solid_signal_set(sig, (CalyndaRtWord)1);
    assert(effect_a_count == 2);
    assert(effect_b_count == 2);

    solid_effect_dispose(ea);
    solid_effect_dispose(eb);
    solid_signal_dispose(sig);
    printf("  [PASS] test_multiple_subscribers\n");
}

static void test_disposed_effect_not_run(void)
{
    SolidSignal *sig = solid_create_signal((CalyndaRtWord)0);
    effect_run_count = 0;

    SolidEffect *eff = solid_create_effect(test_effect_fn, sig);
    assert(effect_run_count == 1);

    solid_effect_dispose(eff);

    solid_signal_set(sig, (CalyndaRtWord)99);
    /* Effect was disposed, so it should not run again */
    assert(effect_run_count == 1);

    solid_signal_dispose(sig);
    printf("  [PASS] test_disposed_effect_not_run\n");
}

/* ---- Main ---- */

int main(void)
{
    printf("=== test_solid_signal ===\n");
    test_create_and_get();
    test_set_changes_value();
    test_auto_tracking();
    test_notification();
    test_no_notification_same_value();
    test_multiple_subscribers();
    test_disposed_effect_not_run();
    printf("=== All signal tests passed ===\n");
    return 0;
}
