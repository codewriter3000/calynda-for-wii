/*
 * calynda_motion.c — Motion input implementation.
 */

#include "calynda_motion.h"
#include <math.h>
#include <string.h>

/* ================================================================== */
/* Internal helpers                                                     */
/* ================================================================== */

static inline float motion_clamp(float v, float lo, float hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

static inline float motion_lerp(float a, float b, float t) {
    return a + (b - a) * t;
}

/* ================================================================== */
/* Init                                                                 */
/* ================================================================== */

void motion_init(MotionSystem *sys) {
    memset(sys, 0, sizeof(*sys));
    for (int i = 0; i < MOTION_MAX_CHANNELS; i++) {
        MotionChannel *ch = &sys->channels[i];
        ch->channel = i;
        ch->active = false;

        /* Default orientation */
        ch->orient.orientation = cm_quat_identity();
        ch->orient.complementary_alpha = 0.98f;
        ch->orient.gravity_estimate = cm_vec3(0, -1, 0);

        /* Default gesture thresholds */
        ch->gesture_det.swing_threshold = 2.5f;   /* in g */
        ch->gesture_det.thrust_threshold = 2.0f;
        ch->gesture_det.twist_threshold = 300.0f;  /* deg/s */
        ch->gesture_det.shake_threshold = 3.0f;
        ch->gesture_det.cooldown_duration = 0.3f;
    }
}

/* ================================================================== */
/* Raw Input Feed                                                       */
/* ================================================================== */

void motion_feed_raw(MotionChannel *ch, CmVec3 accel, CmVec3 gyro, bool mp_connected) {
    ch->raw.accel = accel;
    ch->raw.gyro = gyro;
    ch->raw.mp_connected = mp_connected;
    ch->active = true;
}

void motion_feed_ir(MotionChannel *ch, float raw_x, float raw_y, bool visible) {
    ch->ir.raw_x = raw_x;
    ch->ir.raw_y = raw_y;
    ch->ir.visible = visible;
}

/* ================================================================== */
/* Complementary Filter — Orientation Fusion                            */
/* ================================================================== */

static void update_orientation(MotionChannel *ch, float dt) {
    MotionOrientation *o = &ch->orient;
    CmVec3 accel = ch->raw.accel;
    CmVec3 gyro = ch->raw.gyro;
    float alpha = o->complementary_alpha;

    if (ch->raw.mp_connected) {
        /* Integrate gyroscope (in deg/s → rad/s) */
        CmVec3 gyro_rad;
        gyro_rad.x = gyro.x * CM_DEG2RAD;
        gyro_rad.y = gyro.y * CM_DEG2RAD;
        gyro_rad.z = gyro.z * CM_DEG2RAD;

        o->angular_velocity = gyro;

        /* Small-angle quaternion from angular velocity */
        float half_dt = 0.5f * dt;
        CmQuat dq;
        dq.x = gyro_rad.x * half_dt;
        dq.y = gyro_rad.y * half_dt;
        dq.z = gyro_rad.z * half_dt;
        dq.w = 1.0f;
        dq = cm_quat_normalize(dq);

        /* Gyro-predicted orientation */
        CmQuat gyro_q = cm_quat_mul(o->orientation, dq);

        /* Accelerometer-derived orientation (pitch + roll only) */
        float accel_len = cm_vec3_length(accel);
        if (accel_len > 0.5f && accel_len < 2.0f) {
            /* Normalize accelerometer */
            CmVec3 a_norm = cm_vec3_scale(accel, 1.0f / accel_len);

            /* Complementary filter: blend gyro (high-pass) with accel (low-pass) */
            /* Use alpha for gyro, (1-alpha) for accel correction */

            /* Compute the rotation needed to align accel reading with down vector */
            CmVec3 down = cm_vec3(0, -1, 0);
            CmVec3 accel_up = cm_vec3_neg(a_norm);

            /* Cross product gives rotation axis, dot gives cosine of angle */
            CmVec3 axis = cm_vec3_cross(accel_up, down);
            float axis_len = cm_vec3_length(axis);
            float dot = cm_vec3_dot(accel_up, down);

            if (axis_len > CM_EPSILON) {
                axis = cm_vec3_scale(axis, 1.0f / axis_len);
                float angle = atan2f(axis_len, dot);

                /* Small correction quaternion */
                float corr_amount = (1.0f - alpha) * angle;
                CmQuat correction = cm_quat_from_axis_angle(axis, corr_amount);
                o->orientation = cm_quat_normalize(cm_quat_mul(correction, gyro_q));
            } else {
                o->orientation = cm_quat_normalize(gyro_q);
            }

            o->gravity_estimate = a_norm;
        } else {
            o->orientation = cm_quat_normalize(gyro_q);
        }
    } else {
        /* No MotionPlus: accel-only orientation (noisy, no yaw) */
        float accel_len = cm_vec3_length(accel);
        if (accel_len > 0.5f) {
            CmVec3 a_norm = cm_vec3_scale(accel, 1.0f / accel_len);
            o->gravity_estimate = a_norm;

            /* Derive pitch and roll from accelerometer */
            float pitch = atan2f(-a_norm.z, sqrtf(a_norm.x * a_norm.x + a_norm.y * a_norm.y));
            float roll = atan2f(a_norm.x, -a_norm.y);

            CmQuat qp = cm_quat_from_axis_angle(cm_vec3(1, 0, 0), pitch);
            CmQuat qr = cm_quat_from_axis_angle(cm_vec3(0, 0, 1), roll);
            CmQuat target = cm_quat_normalize(cm_quat_mul(qr, qp));

            /* Smooth toward target */
            o->orientation = cm_quat_slerp(o->orientation, target, 0.1f);
        }

        o->angular_velocity = cm_vec3(0, 0, 0);
    }

    /* Compute linear acceleration (remove gravity) */
    o->linear_accel = cm_vec3_sub(accel, cm_vec3_scale(o->gravity_estimate, -1.0f));
}

/* ================================================================== */
/* Gesture Detection                                                    */
/* ================================================================== */

static void push_history(MotionGestureDetector *det, CmVec3 accel, CmVec3 gyro) {
    det->accel_history[det->history_index] = accel;
    det->gyro_history[det->history_index] = gyro;
    det->history_index = (det->history_index + 1) % MOTION_GESTURE_HISTORY;
    if (det->history_count < MOTION_GESTURE_HISTORY)
        det->history_count++;
}

static CmVec3 history_peak(const CmVec3 *history, int count, int start) {
    CmVec3 peak = cm_vec3(0, 0, 0);
    float peak_mag2 = 0;
    for (int i = 0; i < count; i++) {
        int idx = (start + MOTION_GESTURE_HISTORY - i) % MOTION_GESTURE_HISTORY;
        float mag2 = cm_vec3_dot(history[idx], history[idx]);
        if (mag2 > peak_mag2) {
            peak_mag2 = mag2;
            peak = history[idx];
        }
    }
    return peak;
}

static float history_variance(const CmVec3 *history, int count, int start) {
    if (count < 2) return 0;
    CmVec3 sum = cm_vec3(0, 0, 0);
    for (int i = 0; i < count; i++) {
        int idx = (start + MOTION_GESTURE_HISTORY - i) % MOTION_GESTURE_HISTORY;
        sum = cm_vec3_add(sum, history[idx]);
    }
    CmVec3 mean = cm_vec3_scale(sum, 1.0f / count);

    float var = 0;
    for (int i = 0; i < count; i++) {
        int idx = (start + MOTION_GESTURE_HISTORY - i) % MOTION_GESTURE_HISTORY;
        CmVec3 d = cm_vec3_sub(history[idx], mean);
        var += cm_vec3_dot(d, d);
    }
    return var / count;
}

static void detect_gestures(MotionChannel *ch, float dt) {
    MotionGestureDetector *det = &ch->gesture_det;

    push_history(det, ch->raw.accel, ch->raw.gyro);

    /* Always clear gesture at start of frame */
    det->last_gesture.type = GESTURE_NONE;
    det->last_gesture.active = false;

    /* Cooldown */
    if (det->cooldown_timer > 0) {
        det->cooldown_timer -= dt;
        if (det->cooldown_timer > 0) return;
    }

    if (det->history_count < 5) return; /* need some history */

    int prev_idx = (det->history_index + MOTION_GESTURE_HISTORY - 1) % MOTION_GESTURE_HISTORY;
    int window = det->history_count < 10 ? det->history_count : 10;

    /* Check for shake first (high variance in acceleration) */
    float var = history_variance(det->accel_history, window, prev_idx);
    if (var > det->shake_threshold * det->shake_threshold) {
        det->last_gesture.type = GESTURE_SHAKE;
        det->last_gesture.intensity = motion_clamp(sqrtf(var) / 6.0f, 0, 1);
        det->last_gesture.direction = cm_vec3(0, 0, 0);
        det->last_gesture.duration = window * dt;
        det->last_gesture.active = true;
        det->cooldown_timer = det->cooldown_duration;
        return;
    }

    /* Peak acceleration in recent window */
    CmVec3 peak_accel = history_peak(det->accel_history, window, prev_idx);
    float peak_mag = cm_vec3_length(peak_accel);

    /* Check twist (gyro Z axis) */
    CmVec3 peak_gyro = history_peak(det->gyro_history, window, prev_idx);
    if (fabsf(peak_gyro.z) > det->twist_threshold) {
        det->last_gesture.type = peak_gyro.z > 0 ? GESTURE_TWIST_CCW : GESTURE_TWIST_CW;
        det->last_gesture.intensity = motion_clamp(fabsf(peak_gyro.z) / 600.0f, 0, 1);
        det->last_gesture.direction = cm_vec3(0, 0, peak_gyro.z > 0 ? 1.0f : -1.0f);
        det->last_gesture.duration = window * dt;
        det->last_gesture.active = true;
        det->cooldown_timer = det->cooldown_duration;
        return;
    }

    if (peak_mag < det->swing_threshold) return; /* not enough motion */

    /* Classify swing direction based on dominant axis */
    float ax = fabsf(peak_accel.x);
    float ay = fabsf(peak_accel.y);
    float az = fabsf(peak_accel.z);

    if (az > ax && az > ay && az > det->thrust_threshold) {
        /* Forward/backward — thrust or throw */
        if (peak_accel.y > 0.5f && peak_accel.z < 0) {
            det->last_gesture.type = GESTURE_THROW;
        } else {
            det->last_gesture.type = GESTURE_THRUST;
        }
        det->last_gesture.direction = cm_vec3(0, 0, peak_accel.z > 0 ? 1.0f : -1.0f);
    } else if (ax > ay) {
        /* Left/right swing */
        det->last_gesture.type = peak_accel.x > 0 ? GESTURE_SWING_RIGHT : GESTURE_SWING_LEFT;
        det->last_gesture.direction = cm_vec3(peak_accel.x > 0 ? 1.0f : -1.0f, 0, 0);
    } else {
        /* Up/down swing */
        det->last_gesture.type = peak_accel.y > 0 ? GESTURE_SWING_UP : GESTURE_SWING_DOWN;
        det->last_gesture.direction = cm_vec3(0, peak_accel.y > 0 ? 1.0f : -1.0f, 0);
    }

    det->last_gesture.intensity = motion_clamp(peak_mag / 5.0f, 0, 1);
    det->last_gesture.duration = window * dt;
    det->last_gesture.active = true;
    det->cooldown_timer = det->cooldown_duration;
}

/* ================================================================== */
/* IR Pointer                                                           */
/* ================================================================== */

static void update_ir(MotionChannel *ch) {
    MotionIRPointer *ir = &ch->ir;

    if (!ir->visible) return;

    /* Clamp raw to 0..1 */
    float rx = motion_clamp(ir->raw_x, 0, 1);
    float ry = motion_clamp(ir->raw_y, 0, 1);

    /* Exponential smoothing */
    ir->screen_x = motion_lerp(ir->screen_x, rx, MOTION_IR_SMOOTH_FACTOR);
    ir->screen_y = motion_lerp(ir->screen_y, ry, MOTION_IR_SMOOTH_FACTOR);

    /* Compute aim ray: unproject screen point through a virtual camera */
    /* Map 0..1 to -1..1 NDC */
    float ndc_x = ir->screen_x * 2.0f - 1.0f;
    float ndc_y = -(ir->screen_y * 2.0f - 1.0f); /* flip Y */

    /* Assume a camera at origin looking down -Z with 60 degree FOV */
    float fov_half_tan = tanf(30.0f * CM_DEG2RAD);
    float aspect = 640.0f / 480.0f;

    CmVec3 dir;
    dir.x = ndc_x * fov_half_tan * aspect;
    dir.y = ndc_y * fov_half_tan;
    dir.z = -1.0f;
    dir = cm_vec3_normalize(dir);

    ir->aim_ray.origin = cm_vec3(0, 0, 0);
    ir->aim_ray.direction = dir;
}

/* ================================================================== */
/* Main Update                                                          */
/* ================================================================== */

void motion_update(MotionChannel *ch, float dt) {
    if (!ch->active) return;

    update_orientation(ch, dt);
    detect_gestures(ch, dt);
    update_ir(ch);
}

/* ================================================================== */
/* Query API                                                            */
/* ================================================================== */

CmQuat motion_get_orientation(const MotionChannel *ch) {
    return ch->orient.orientation;
}

CmVec3 motion_get_angular_velocity(const MotionChannel *ch) {
    return ch->orient.angular_velocity;
}

CmVec3 motion_get_linear_accel(const MotionChannel *ch) {
    return ch->orient.linear_accel;
}

MotionGesture motion_get_gesture(const MotionChannel *ch) {
    return ch->gesture_det.last_gesture;
}

void motion_get_pointer(const MotionChannel *ch, float *x, float *y, bool *visible) {
    *x = ch->ir.screen_x;
    *y = ch->ir.screen_y;
    *visible = ch->ir.visible;
}

CmRay motion_get_aim_ray(const MotionChannel *ch) {
    return ch->ir.aim_ray;
}

void motion_set_swing_threshold(MotionChannel *ch, float threshold) {
    ch->gesture_det.swing_threshold = threshold;
}

void motion_set_thrust_threshold(MotionChannel *ch, float threshold) {
    ch->gesture_det.thrust_threshold = threshold;
}

void motion_set_twist_threshold(MotionChannel *ch, float threshold) {
    ch->gesture_det.twist_threshold = threshold;
}

/* ================================================================== */
/* Calibration                                                          */
/* ================================================================== */

void motion_calibrate(MotionChannel *ch) {
    /* Use current accel reading as gravity reference */
    float len = cm_vec3_length(ch->raw.accel);
    if (len > 0.5f) {
        ch->orient.gravity_estimate = cm_vec3_scale(ch->raw.accel, 1.0f / len);
    }
    ch->orient.orientation = cm_quat_identity();
    ch->orient.angular_velocity = cm_vec3(0, 0, 0);
    ch->orient.linear_accel = cm_vec3(0, 0, 0);
}

void motion_reset_orientation(MotionChannel *ch) {
    ch->orient.orientation = cm_quat_identity();
}
