/*
 * test_motion.c — Unit tests for calynda_motion.
 */

#include "calynda_motion.h"
#include <stdio.h>
#include <math.h>
#include <string.h>

/* ================================================================== */
/* Test framework                                                       */
/* ================================================================== */

static int tests_run = 0, tests_passed = 0, tests_failed = 0;

#define ASSERT_TRUE(expr) do { \
    tests_run++; \
    if (!(expr)) { \
        printf("  FAIL %s:%d: %s\n", __FILE__, __LINE__, #expr); \
        tests_failed++; return; \
    } else { tests_passed++; } \
} while(0)

#define ASSERT_NEAR(a, b, eps) do { \
    tests_run++; \
    if (fabsf((float)(a) - (float)(b)) > (eps)) { \
        printf("  FAIL %s:%d: %s ~= %s (%.6f vs %.6f)\n", \
               __FILE__, __LINE__, #a, #b, (float)(a), (float)(b)); \
        tests_failed++; return; \
    } else { tests_passed++; } \
} while(0)

#define RUN_TEST(fn) do { \
    int _before = tests_failed; \
    fn(); \
    printf("  %s: %s\n", (tests_failed == _before ? "PASS" : "FAIL"), #fn); \
} while(0)

/* ================================================================== */
/* System Init tests                                                    */
/* ================================================================== */

static void test_init(void) {
    MotionSystem sys;
    motion_init(&sys);
    ASSERT_TRUE(sys.active_count == 0);
    ASSERT_TRUE(sys.channels[0].channel == 0);
    ASSERT_TRUE(sys.channels[1].channel == 1);
    ASSERT_TRUE(!sys.channels[0].active);

    /* Default orientation should be identity */
    CmQuat q = sys.channels[0].orient.orientation;
    ASSERT_NEAR(q.w, 1.0f, 0.001f);
    ASSERT_NEAR(q.x, 0, 0.001f);
    ASSERT_NEAR(q.y, 0, 0.001f);
    ASSERT_NEAR(q.z, 0, 0.001f);
}

static void test_default_thresholds(void) {
    MotionSystem sys;
    motion_init(&sys);
    MotionChannel *ch = &sys.channels[0];
    ASSERT_NEAR(ch->gesture_det.swing_threshold, 2.5f, 0.01f);
    ASSERT_NEAR(ch->gesture_det.thrust_threshold, 2.0f, 0.01f);
    ASSERT_NEAR(ch->gesture_det.twist_threshold, 300.0f, 0.01f);
    ASSERT_NEAR(ch->orient.complementary_alpha, 0.98f, 0.01f);
}

/* ================================================================== */
/* Raw input tests                                                      */
/* ================================================================== */

static void test_feed_raw(void) {
    MotionSystem sys;
    motion_init(&sys);
    MotionChannel *ch = &sys.channels[0];

    motion_feed_raw(ch, cm_vec3(0, -1, 0), cm_vec3(10, 20, 30), true);
    ASSERT_TRUE(ch->active);
    ASSERT_NEAR(ch->raw.accel.y, -1.0f, 0.001f);
    ASSERT_NEAR(ch->raw.gyro.x, 10.0f, 0.001f);
    ASSERT_TRUE(ch->raw.mp_connected);
}

static void test_feed_ir(void) {
    MotionSystem sys;
    motion_init(&sys);
    MotionChannel *ch = &sys.channels[0];

    motion_feed_ir(ch, 0.5f, 0.3f, true);
    ASSERT_NEAR(ch->ir.raw_x, 0.5f, 0.001f);
    ASSERT_NEAR(ch->ir.raw_y, 0.3f, 0.001f);
    ASSERT_TRUE(ch->ir.visible);
}

/* ================================================================== */
/* Orientation fusion tests                                             */
/* ================================================================== */

static void test_stationary_orientation(void) {
    MotionSystem sys;
    motion_init(&sys);
    MotionChannel *ch = &sys.channels[0];

    /* Feed stationary data: gravity pointing down, no rotation */
    for (int i = 0; i < 60; i++) {
        motion_feed_raw(ch, cm_vec3(0, -1, 0), cm_vec3(0, 0, 0), true);
        motion_update(ch, 1.0f / 60.0f);
    }

    /* Orientation should remain close to identity */
    CmQuat q = motion_get_orientation(ch);
    ASSERT_NEAR(q.w, 1.0f, 0.15f);
}

static void test_rotation_with_gyro(void) {
    MotionSystem sys;
    motion_init(&sys);
    MotionChannel *ch = &sys.channels[0];

    /* Feed rotation around Y axis at 90 deg/s for 1 second */
    for (int i = 0; i < 60; i++) {
        motion_feed_raw(ch, cm_vec3(0, -1, 0), cm_vec3(0, 90, 0), true);
        motion_update(ch, 1.0f / 60.0f);
    }

    /* Should have rotated roughly 90 degrees around Y */
    CmQuat q = motion_get_orientation(ch);
    /* After 90deg Y rotation, the quaternion should be approximately (0, sin(45deg), 0, cos(45deg)) */
    float expected_w = cosf(45.0f * CM_DEG2RAD);
    /* Allow generous tolerance since complementary filter drifts toward accel */
    ASSERT_NEAR(fabsf(q.w), expected_w, 0.3f);
}

static void test_accel_only_orientation(void) {
    MotionSystem sys;
    motion_init(&sys);
    MotionChannel *ch = &sys.channels[0];

    /* No MotionPlus — accel only */
    for (int i = 0; i < 60; i++) {
        motion_feed_raw(ch, cm_vec3(0, -1, 0), cm_vec3(0, 0, 0), false);
        motion_update(ch, 1.0f / 60.0f);
    }

    /* Should converge to identity-ish orientation */
    CmQuat q = motion_get_orientation(ch);
    ASSERT_NEAR(q.w, 1.0f, 0.3f);
}

static void test_linear_accel_extraction(void) {
    MotionSystem sys;
    motion_init(&sys);
    MotionChannel *ch = &sys.channels[0];

    /* Feed gravity + extra acceleration in X */
    motion_feed_raw(ch, cm_vec3(2.0f, -1, 0), cm_vec3(0, 0, 0), true);
    motion_update(ch, 1.0f / 60.0f);

    CmVec3 lin = motion_get_linear_accel(ch);
    /* linear accel should have a significant X component */
    ASSERT_TRUE(fabsf(lin.x) > 0.5f);
}

/* ================================================================== */
/* Gesture detection tests                                              */
/* ================================================================== */

static void test_gesture_none_when_still(void) {
    MotionSystem sys;
    motion_init(&sys);
    MotionChannel *ch = &sys.channels[0];

    for (int i = 0; i < 30; i++) {
        motion_feed_raw(ch, cm_vec3(0, -1, 0), cm_vec3(0, 0, 0), true);
        motion_update(ch, 1.0f / 60.0f);
    }

    MotionGesture g = motion_get_gesture(ch);
    ASSERT_TRUE(g.type == GESTURE_NONE);
}

static void test_gesture_swing_right(void) {
    MotionSystem sys;
    motion_init(&sys);
    MotionChannel *ch = &sys.channels[0];

    /* Fill some baseline history */
    for (int i = 0; i < 10; i++) {
        motion_feed_raw(ch, cm_vec3(0, -1, 0), cm_vec3(0, 0, 0), true);
        motion_update(ch, 1.0f / 60.0f);
    }

    /* Sharp acceleration in +X (right swing) — check each frame */
    MotionGesture g = {0};
    for (int i = 0; i < 5; i++) {
        motion_feed_raw(ch, cm_vec3(4.0f, -1, 0), cm_vec3(0, 0, 0), true);
        motion_update(ch, 1.0f / 60.0f);
        g = motion_get_gesture(ch);
        if (g.type != GESTURE_NONE) break;
    }

    ASSERT_TRUE(g.type == GESTURE_SWING_RIGHT);
    ASSERT_TRUE(g.intensity > 0);
    ASSERT_TRUE(g.active);
}

static void test_gesture_swing_left(void) {
    MotionSystem sys;
    motion_init(&sys);
    MotionChannel *ch = &sys.channels[0];

    for (int i = 0; i < 10; i++) {
        motion_feed_raw(ch, cm_vec3(0, -1, 0), cm_vec3(0, 0, 0), true);
        motion_update(ch, 1.0f / 60.0f);
    }

    MotionGesture g = {0};
    for (int i = 0; i < 5; i++) {
        motion_feed_raw(ch, cm_vec3(-4.0f, -1, 0), cm_vec3(0, 0, 0), true);
        motion_update(ch, 1.0f / 60.0f);
        g = motion_get_gesture(ch);
        if (g.type != GESTURE_NONE) break;
    }

    ASSERT_TRUE(g.type == GESTURE_SWING_LEFT);
}

static void test_gesture_thrust(void) {
    MotionSystem sys;
    motion_init(&sys);
    MotionChannel *ch = &sys.channels[0];

    for (int i = 0; i < 10; i++) {
        motion_feed_raw(ch, cm_vec3(0, -1, 0), cm_vec3(0, 0, 0), true);
        motion_update(ch, 1.0f / 60.0f);
    }

    MotionGesture g = {0};
    for (int i = 0; i < 5; i++) {
        motion_feed_raw(ch, cm_vec3(0, -1, -4.0f), cm_vec3(0, 0, 0), true);
        motion_update(ch, 1.0f / 60.0f);
        g = motion_get_gesture(ch);
        if (g.type != GESTURE_NONE) break;
    }

    ASSERT_TRUE(g.type == GESTURE_THRUST);
}

static void test_gesture_twist(void) {
    MotionSystem sys;
    motion_init(&sys);
    MotionChannel *ch = &sys.channels[0];

    for (int i = 0; i < 10; i++) {
        motion_feed_raw(ch, cm_vec3(0, -1, 0), cm_vec3(0, 0, 0), true);
        motion_update(ch, 1.0f / 60.0f);
    }

    MotionGesture g = {0};
    for (int i = 0; i < 5; i++) {
        motion_feed_raw(ch, cm_vec3(0, -1, 0), cm_vec3(0, 0, 400), true);
        motion_update(ch, 1.0f / 60.0f);
        g = motion_get_gesture(ch);
        if (g.type != GESTURE_NONE) break;
    }

    ASSERT_TRUE(g.type == GESTURE_TWIST_CCW);
    ASSERT_TRUE(g.intensity > 0);
}

static void test_gesture_cooldown(void) {
    MotionSystem sys;
    motion_init(&sys);
    MotionChannel *ch = &sys.channels[0];

    for (int i = 0; i < 10; i++) {
        motion_feed_raw(ch, cm_vec3(0, -1, 0), cm_vec3(0, 0, 0), true);
        motion_update(ch, 1.0f / 60.0f);
    }

    /* Trigger a gesture */
    motion_feed_raw(ch, cm_vec3(4.0f, -1, 0), cm_vec3(0, 0, 0), true);
    motion_update(ch, 1.0f / 60.0f);

    MotionGesture g1 = motion_get_gesture(ch);
    ASSERT_TRUE(g1.type == GESTURE_SWING_RIGHT);

    /* Next frame during cooldown should be NONE */
    motion_feed_raw(ch, cm_vec3(-4.0f, -1, 0), cm_vec3(0, 0, 0), true);
    motion_update(ch, 1.0f / 60.0f);

    MotionGesture g2 = motion_get_gesture(ch);
    ASSERT_TRUE(g2.type == GESTURE_NONE);
}

static void test_gesture_shake(void) {
    MotionSystem sys;
    motion_init(&sys);
    MotionChannel *ch = &sys.channels[0];

    /* Rapid alternating acceleration (shake) */
    MotionGesture g = {0};
    for (int i = 0; i < 20; i++) {
        float dir = (i % 2 == 0) ? 5.0f : -5.0f;
        motion_feed_raw(ch, cm_vec3(dir, -1 + dir * 0.5f, dir * 0.3f), cm_vec3(0, 0, 0), true);
        motion_update(ch, 1.0f / 60.0f);
        g = motion_get_gesture(ch);
        if (g.type == GESTURE_SHAKE) break;
    }

    ASSERT_TRUE(g.type == GESTURE_SHAKE);
}

/* ================================================================== */
/* IR Pointer tests                                                     */
/* ================================================================== */

static void test_ir_smoothing(void) {
    MotionSystem sys;
    motion_init(&sys);
    MotionChannel *ch = &sys.channels[0];
    ch->active = true;

    /* Feed IR at center */
    for (int i = 0; i < 30; i++) {
        motion_feed_raw(ch, cm_vec3(0, -1, 0), cm_vec3(0, 0, 0), false);
        motion_feed_ir(ch, 0.5f, 0.5f, true);
        motion_update(ch, 1.0f / 60.0f);
    }

    float x, y;
    bool visible;
    motion_get_pointer(ch, &x, &y, &visible);
    ASSERT_TRUE(visible);
    ASSERT_NEAR(x, 0.5f, 0.1f);
    ASSERT_NEAR(y, 0.5f, 0.1f);
}

static void test_ir_not_visible(void) {
    MotionSystem sys;
    motion_init(&sys);
    MotionChannel *ch = &sys.channels[0];
    ch->active = true;

    motion_feed_ir(ch, 0, 0, false);

    float x, y;
    bool visible;
    motion_get_pointer(ch, &x, &y, &visible);
    ASSERT_TRUE(!visible);
}

static void test_aim_ray(void) {
    MotionSystem sys;
    motion_init(&sys);
    MotionChannel *ch = &sys.channels[0];
    ch->active = true;

    /* Point at center of screen */
    for (int i = 0; i < 30; i++) {
        motion_feed_raw(ch, cm_vec3(0, -1, 0), cm_vec3(0, 0, 0), false);
        motion_feed_ir(ch, 0.5f, 0.5f, true);
        motion_update(ch, 1.0f / 60.0f);
    }

    CmRay ray = motion_get_aim_ray(ch);
    /* Ray should point roughly down -Z when pointing at center */
    ASSERT_TRUE(ray.direction.z < -0.5f);
    ASSERT_NEAR(ray.direction.x, 0, 0.1f);
    ASSERT_NEAR(ray.direction.y, 0, 0.1f);
}

/* ================================================================== */
/* Threshold setting tests                                              */
/* ================================================================== */

static void test_set_thresholds(void) {
    MotionSystem sys;
    motion_init(&sys);
    MotionChannel *ch = &sys.channels[0];

    motion_set_swing_threshold(ch, 3.0f);
    motion_set_thrust_threshold(ch, 2.5f);
    motion_set_twist_threshold(ch, 350.0f);

    ASSERT_NEAR(ch->gesture_det.swing_threshold, 3.0f, 0.001f);
    ASSERT_NEAR(ch->gesture_det.thrust_threshold, 2.5f, 0.001f);
    ASSERT_NEAR(ch->gesture_det.twist_threshold, 350.0f, 0.001f);
}

/* ================================================================== */
/* Calibration tests                                                    */
/* ================================================================== */

static void test_calibrate(void) {
    MotionSystem sys;
    motion_init(&sys);
    MotionChannel *ch = &sys.channels[0];

    /* Feed some rotation */
    for (int i = 0; i < 30; i++) {
        motion_feed_raw(ch, cm_vec3(0, -1, 0), cm_vec3(0, 45, 0), true);
        motion_update(ch, 1.0f / 60.0f);
    }

    /* Orientation should have drifted from identity */
    CmQuat before = motion_get_orientation(ch);
    ASSERT_TRUE(fabsf(before.w) < 0.99f);

    /* Calibrate resets */
    motion_calibrate(ch);
    CmQuat after = motion_get_orientation(ch);
    ASSERT_NEAR(after.w, 1.0f, 0.001f);
}

static void test_reset_orientation(void) {
    MotionSystem sys;
    motion_init(&sys);
    MotionChannel *ch = &sys.channels[0];

    /* Rotate */
    for (int i = 0; i < 30; i++) {
        motion_feed_raw(ch, cm_vec3(0, -1, 0), cm_vec3(90, 0, 0), true);
        motion_update(ch, 1.0f / 60.0f);
    }

    motion_reset_orientation(ch);
    CmQuat q = motion_get_orientation(ch);
    ASSERT_NEAR(q.w, 1.0f, 0.001f);
    ASSERT_NEAR(q.x, 0, 0.001f);
}

/* ================================================================== */
/* Main                                                                 */
/* ================================================================== */

int main(void) {
    printf("=== calynda_motion tests ===\n");

    printf("\n--- Init ---\n");
    RUN_TEST(test_init);
    RUN_TEST(test_default_thresholds);

    printf("\n--- Raw Input ---\n");
    RUN_TEST(test_feed_raw);
    RUN_TEST(test_feed_ir);

    printf("\n--- Orientation ---\n");
    RUN_TEST(test_stationary_orientation);
    RUN_TEST(test_rotation_with_gyro);
    RUN_TEST(test_accel_only_orientation);
    RUN_TEST(test_linear_accel_extraction);

    printf("\n--- Gestures ---\n");
    RUN_TEST(test_gesture_none_when_still);
    RUN_TEST(test_gesture_swing_right);
    RUN_TEST(test_gesture_swing_left);
    RUN_TEST(test_gesture_thrust);
    RUN_TEST(test_gesture_twist);
    RUN_TEST(test_gesture_cooldown);
    RUN_TEST(test_gesture_shake);

    printf("\n--- IR Pointer ---\n");
    RUN_TEST(test_ir_smoothing);
    RUN_TEST(test_ir_not_visible);
    RUN_TEST(test_aim_ray);

    printf("\n--- Settings ---\n");
    RUN_TEST(test_set_thresholds);
    RUN_TEST(test_calibrate);
    RUN_TEST(test_reset_orientation);

    printf("\n===========================\n");
    printf("Total: %d  Passed: %d  Failed: %d\n", tests_run, tests_passed, tests_failed);
    return tests_failed > 0 ? 1 : 0;
}
