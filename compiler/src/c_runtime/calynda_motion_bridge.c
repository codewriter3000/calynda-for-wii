/*
 * calynda_motion_bridge.c — Motion bridge: import game.motion;
 *
 * Callable kind range: 500–599
 */

#include "calynda_motion_bridge.h"

#include <stdio.h>
#include <string.h>

#ifdef CALYNDA_WII_BUILD
#include "calynda_motion.h"
#include <math.h>
#endif

/* ------------------------------------------------------------------ */
/* Float ↔ Word                                                         */
/* ------------------------------------------------------------------ */

static inline float word_to_float(CalyndaRtWord w) {
    union { CalyndaRtWord w; float f; } u;
    u.w = w;
    return u.f;
}

static inline CalyndaRtWord float_to_word(float f) {
    union { float f; CalyndaRtWord w; } u;
    u.w = 0;
    u.f = f;
    return u.w;
}

/* ------------------------------------------------------------------ */
/* Package                                                              */
/* ------------------------------------------------------------------ */

CalyndaRtPackage __calynda_pkg_motion = {
    { CALYNDA_RT_OBJECT_MAGIC, CALYNDA_RT_OBJECT_PACKAGE },
    "motion"
};

/* ------------------------------------------------------------------ */
/* Callable IDs                                                         */
/* ------------------------------------------------------------------ */

enum {
    MOT_CALL_INIT = 500,
    MOT_CALL_UPDATE,
    MOT_CALL_FEED_RAW,
    MOT_CALL_FEED_IR,

    /* Queries */
    MOT_CALL_GET_PITCH,
    MOT_CALL_GET_YAW,
    MOT_CALL_GET_ROLL,
    MOT_CALL_GET_ANGULAR_VEL_X,
    MOT_CALL_GET_ANGULAR_VEL_Y,
    MOT_CALL_GET_ANGULAR_VEL_Z,
    MOT_CALL_GET_LINEAR_ACCEL_X,
    MOT_CALL_GET_LINEAR_ACCEL_Y,
    MOT_CALL_GET_LINEAR_ACCEL_Z,
    MOT_CALL_GET_GESTURE,
    MOT_CALL_GET_GESTURE_STRENGTH,
    MOT_CALL_GET_POINTER_X,
    MOT_CALL_GET_POINTER_Y,
    MOT_CALL_GET_AIM_RAY_OX,
    MOT_CALL_GET_AIM_RAY_OY,
    MOT_CALL_GET_AIM_RAY_OZ,
    MOT_CALL_GET_AIM_RAY_DX,
    MOT_CALL_GET_AIM_RAY_DY,
    MOT_CALL_GET_AIM_RAY_DZ,

    /* Config */
    MOT_CALL_SET_THRESHOLDS,
    MOT_CALL_CALIBRATE,
    MOT_CALL_RESET_ORIENTATION,

    /* Gesture constants */
    MOT_CALL_GESTURE_NONE,
    MOT_CALL_GESTURE_SWING_LEFT,
    MOT_CALL_GESTURE_SWING_RIGHT,
    MOT_CALL_GESTURE_SWING_UP,
    MOT_CALL_GESTURE_SWING_DOWN,
    MOT_CALL_GESTURE_THRUST,
    MOT_CALL_GESTURE_THROW,
    MOT_CALL_GESTURE_TWIST_CW,
    MOT_CALL_GESTURE_TWIST_CCW,
    MOT_CALL_GESTURE_SHAKE,
};

/* ------------------------------------------------------------------ */
/* Callable Objects                                                     */
/* ------------------------------------------------------------------ */

#define MOT_CALLABLE(var, id, label) \
    static CalyndaRtExternCallable var = { \
        { CALYNDA_RT_OBJECT_MAGIC, CALYNDA_RT_OBJECT_EXTERN_CALLABLE }, \
        (CalyndaRtExternCallableKind)(id), \
        label \
    }

MOT_CALLABLE(MOT_INIT_CALLABLE,        MOT_CALL_INIT,         "init");
MOT_CALLABLE(MOT_UPDATE_CALLABLE,      MOT_CALL_UPDATE,       "update");
MOT_CALLABLE(MOT_FEED_RAW_CALLABLE,    MOT_CALL_FEED_RAW,     "feed_raw");
MOT_CALLABLE(MOT_FEED_IR_CALLABLE,     MOT_CALL_FEED_IR,      "feed_ir");
MOT_CALLABLE(MOT_PITCH_CALLABLE,       MOT_CALL_GET_PITCH,    "get_pitch");
MOT_CALLABLE(MOT_YAW_CALLABLE,         MOT_CALL_GET_YAW,      "get_yaw");
MOT_CALLABLE(MOT_ROLL_CALLABLE,        MOT_CALL_GET_ROLL,     "get_roll");
MOT_CALLABLE(MOT_ANG_X_CALLABLE,       MOT_CALL_GET_ANGULAR_VEL_X, "get_angular_vel_x");
MOT_CALLABLE(MOT_ANG_Y_CALLABLE,       MOT_CALL_GET_ANGULAR_VEL_Y, "get_angular_vel_y");
MOT_CALLABLE(MOT_ANG_Z_CALLABLE,       MOT_CALL_GET_ANGULAR_VEL_Z, "get_angular_vel_z");
MOT_CALLABLE(MOT_LIN_X_CALLABLE,       MOT_CALL_GET_LINEAR_ACCEL_X, "get_linear_accel_x");
MOT_CALLABLE(MOT_LIN_Y_CALLABLE,       MOT_CALL_GET_LINEAR_ACCEL_Y, "get_linear_accel_y");
MOT_CALLABLE(MOT_LIN_Z_CALLABLE,       MOT_CALL_GET_LINEAR_ACCEL_Z, "get_linear_accel_z");
MOT_CALLABLE(MOT_GESTURE_CALLABLE,     MOT_CALL_GET_GESTURE,  "get_gesture");
MOT_CALLABLE(MOT_GESTURE_STR_CALLABLE, MOT_CALL_GET_GESTURE_STRENGTH, "get_gesture_strength");
MOT_CALLABLE(MOT_PTR_X_CALLABLE,       MOT_CALL_GET_POINTER_X, "get_pointer_x");
MOT_CALLABLE(MOT_PTR_Y_CALLABLE,       MOT_CALL_GET_POINTER_Y, "get_pointer_y");
MOT_CALLABLE(MOT_AIM_OX_CALLABLE,      MOT_CALL_GET_AIM_RAY_OX, "get_aim_ray_ox");
MOT_CALLABLE(MOT_AIM_OY_CALLABLE,      MOT_CALL_GET_AIM_RAY_OY, "get_aim_ray_oy");
MOT_CALLABLE(MOT_AIM_OZ_CALLABLE,      MOT_CALL_GET_AIM_RAY_OZ, "get_aim_ray_oz");
MOT_CALLABLE(MOT_AIM_DX_CALLABLE,      MOT_CALL_GET_AIM_RAY_DX, "get_aim_ray_dx");
MOT_CALLABLE(MOT_AIM_DY_CALLABLE,      MOT_CALL_GET_AIM_RAY_DY, "get_aim_ray_dy");
MOT_CALLABLE(MOT_AIM_DZ_CALLABLE,      MOT_CALL_GET_AIM_RAY_DZ, "get_aim_ray_dz");
MOT_CALLABLE(MOT_THRESHOLDS_CALLABLE,  MOT_CALL_SET_THRESHOLDS, "set_thresholds");
MOT_CALLABLE(MOT_CALIBRATE_CALLABLE,   MOT_CALL_CALIBRATE,    "calibrate");
MOT_CALLABLE(MOT_RESET_ORI_CALLABLE,   MOT_CALL_RESET_ORIENTATION, "reset_orientation");
MOT_CALLABLE(MOT_G_NONE_CALLABLE,      MOT_CALL_GESTURE_NONE,        "GESTURE_NONE");
MOT_CALLABLE(MOT_G_SWING_L_CALLABLE,   MOT_CALL_GESTURE_SWING_LEFT,  "GESTURE_SWING_LEFT");
MOT_CALLABLE(MOT_G_SWING_R_CALLABLE,   MOT_CALL_GESTURE_SWING_RIGHT, "GESTURE_SWING_RIGHT");
MOT_CALLABLE(MOT_G_SWING_U_CALLABLE,   MOT_CALL_GESTURE_SWING_UP,    "GESTURE_SWING_UP");
MOT_CALLABLE(MOT_G_SWING_D_CALLABLE,   MOT_CALL_GESTURE_SWING_DOWN,  "GESTURE_SWING_DOWN");
MOT_CALLABLE(MOT_G_THRUST_CALLABLE,    MOT_CALL_GESTURE_THRUST,      "GESTURE_THRUST");
MOT_CALLABLE(MOT_G_THROW_CALLABLE,     MOT_CALL_GESTURE_THROW,       "GESTURE_THROW");
MOT_CALLABLE(MOT_G_TWIST_CW_CALLABLE,  MOT_CALL_GESTURE_TWIST_CW,    "GESTURE_TWIST_CW");
MOT_CALLABLE(MOT_G_TWIST_CCW_CALLABLE, MOT_CALL_GESTURE_TWIST_CCW,   "GESTURE_TWIST_CCW");
MOT_CALLABLE(MOT_G_SHAKE_CALLABLE,     MOT_CALL_GESTURE_SHAKE,        "GESTURE_SHAKE");

#undef MOT_CALLABLE

static CalyndaRtExternCallable *ALL_MOT_CALLABLES[] = {
    &MOT_INIT_CALLABLE, &MOT_UPDATE_CALLABLE,
    &MOT_FEED_RAW_CALLABLE, &MOT_FEED_IR_CALLABLE,
    &MOT_PITCH_CALLABLE, &MOT_YAW_CALLABLE, &MOT_ROLL_CALLABLE,
    &MOT_ANG_X_CALLABLE, &MOT_ANG_Y_CALLABLE, &MOT_ANG_Z_CALLABLE,
    &MOT_LIN_X_CALLABLE, &MOT_LIN_Y_CALLABLE, &MOT_LIN_Z_CALLABLE,
    &MOT_GESTURE_CALLABLE, &MOT_GESTURE_STR_CALLABLE,
    &MOT_PTR_X_CALLABLE, &MOT_PTR_Y_CALLABLE,
    &MOT_AIM_OX_CALLABLE, &MOT_AIM_OY_CALLABLE, &MOT_AIM_OZ_CALLABLE,
    &MOT_AIM_DX_CALLABLE, &MOT_AIM_DY_CALLABLE, &MOT_AIM_DZ_CALLABLE,
    &MOT_THRESHOLDS_CALLABLE, &MOT_CALIBRATE_CALLABLE, &MOT_RESET_ORI_CALLABLE,
    &MOT_G_NONE_CALLABLE, &MOT_G_SWING_L_CALLABLE, &MOT_G_SWING_R_CALLABLE,
    &MOT_G_SWING_U_CALLABLE, &MOT_G_SWING_D_CALLABLE,
    &MOT_G_THRUST_CALLABLE, &MOT_G_THROW_CALLABLE,
    &MOT_G_TWIST_CW_CALLABLE, &MOT_G_TWIST_CCW_CALLABLE,
    &MOT_G_SHAKE_CALLABLE,
};
#define MOT_CALLABLE_COUNT (sizeof(ALL_MOT_CALLABLES) / sizeof(ALL_MOT_CALLABLES[0]))

/* ------------------------------------------------------------------ */
/* Registration                                                         */
/* ------------------------------------------------------------------ */

void calynda_motion_register_objects(void) {
    calynda_rt_register_static_object(&__calynda_pkg_motion.header);
    for (size_t i = 0; i < MOT_CALLABLE_COUNT; i++) {
        calynda_rt_register_static_object(&ALL_MOT_CALLABLES[i]->header);
    }
}

/* ------------------------------------------------------------------ */
/* Dispatch                                                             */
/* ------------------------------------------------------------------ */

#ifdef CALYNDA_WII_BUILD
/* ----- Wii: static motion state ----- */
static MotionSystem g_motion_sys;
static bool g_motion_inited = false;
#endif

CalyndaRtWord calynda_motion_dispatch(const CalyndaRtExternCallable *callable,
                                       size_t argument_count,
                                       const CalyndaRtWord *arguments) {
    (void)argument_count;
    (void)arguments;

    switch ((int)callable->kind) {

    case MOT_CALL_INIT:
#ifdef CALYNDA_WII_BUILD
        motion_init(&g_motion_sys);
        g_motion_inited = true;
#else
        fprintf(stderr, "[motion.init] stub\n");
#endif
        return 0;

    case MOT_CALL_UPDATE:
#ifdef CALYNDA_WII_BUILD
        if (g_motion_inited) {
            int ch = (argument_count > 0) ? (int)arguments[0] : 0;
            float dt = (argument_count > 1) ? word_to_float(arguments[1]) : (1.0f / 60.0f);
            if (ch >= 0 && ch < MOTION_MAX_CHANNELS && g_motion_sys.channels[ch].active)
                motion_update(&g_motion_sys.channels[ch], dt);
        }
#else
        fprintf(stderr, "[motion.update] stub\n");
#endif
        return 0;

    case MOT_CALL_FEED_RAW:
#ifdef CALYNDA_WII_BUILD
        /* args: channel, ax, ay, az, gx, gy, gz, mp_connected */
        if (g_motion_inited && argument_count >= 8) {
            int ch = (int)arguments[0];
            CmVec3 accel = cm_vec3(word_to_float(arguments[1]),
                                    word_to_float(arguments[2]),
                                    word_to_float(arguments[3]));
            CmVec3 gyro  = cm_vec3(word_to_float(arguments[4]),
                                    word_to_float(arguments[5]),
                                    word_to_float(arguments[6]));
            bool mp = (bool)arguments[7];
            if (ch >= 0 && ch < MOTION_MAX_CHANNELS) {
                g_motion_sys.channels[ch].active = true;
                motion_feed_raw(&g_motion_sys.channels[ch], accel, gyro, mp);
            }
        }
#else
        fprintf(stderr, "[motion.feed_raw] stub\n");
#endif
        return 0;

    case MOT_CALL_FEED_IR:
#ifdef CALYNDA_WII_BUILD
        /* args: channel, raw_x, raw_y, visible */
        if (g_motion_inited && argument_count >= 4) {
            int ch = (int)arguments[0];
            float rx = word_to_float(arguments[1]);
            float ry = word_to_float(arguments[2]);
            bool vis = (bool)arguments[3];
            if (ch >= 0 && ch < MOTION_MAX_CHANNELS) {
                g_motion_sys.channels[ch].active = true;
                motion_feed_ir(&g_motion_sys.channels[ch], rx, ry, vis);
            }
        }
#else
        fprintf(stderr, "[motion.feed_ir] stub\n");
#endif
        return 0;

    /* ---- Orientation queries (channel 0 default) ---- */
    case MOT_CALL_GET_PITCH: {
#ifdef CALYNDA_WII_BUILD
        int ch = (argument_count > 0) ? (int)arguments[0] : 0;
        if (g_motion_inited && ch >= 0 && ch < MOTION_MAX_CHANNELS) {
            CmQuat q = motion_get_orientation(&g_motion_sys.channels[ch]);
            /* pitch = atan2(2(qw*qx + qy*qz), 1 - 2(qx^2 + qy^2)) */
            float sinp = 2.0f * (q.w * q.x + q.y * q.z);
            float cosp = 1.0f - 2.0f * (q.x * q.x + q.y * q.y);
            float pitch = atan2f(sinp, cosp);
            return float_to_word(pitch);
        }
#endif
        return float_to_word(0);
    }

    case MOT_CALL_GET_YAW: {
#ifdef CALYNDA_WII_BUILD
        int ch = (argument_count > 0) ? (int)arguments[0] : 0;
        if (g_motion_inited && ch >= 0 && ch < MOTION_MAX_CHANNELS) {
            CmQuat q = motion_get_orientation(&g_motion_sys.channels[ch]);
            float siny = 2.0f * (q.w * q.y - q.z * q.x);
            if (siny > 1.0f) siny = 1.0f;
            if (siny < -1.0f) siny = -1.0f;
            float yaw = asinf(siny);
            return float_to_word(yaw);
        }
#endif
        return float_to_word(0);
    }

    case MOT_CALL_GET_ROLL: {
#ifdef CALYNDA_WII_BUILD
        int ch = (argument_count > 0) ? (int)arguments[0] : 0;
        if (g_motion_inited && ch >= 0 && ch < MOTION_MAX_CHANNELS) {
            CmQuat q = motion_get_orientation(&g_motion_sys.channels[ch]);
            float sinr = 2.0f * (q.w * q.z + q.x * q.y);
            float cosr = 1.0f - 2.0f * (q.y * q.y + q.z * q.z);
            float roll = atan2f(sinr, cosr);
            return float_to_word(roll);
        }
#endif
        return float_to_word(0);
    }

    case MOT_CALL_GET_ANGULAR_VEL_X: {
#ifdef CALYNDA_WII_BUILD
        int ch = (argument_count > 0) ? (int)arguments[0] : 0;
        if (g_motion_inited && ch >= 0 && ch < MOTION_MAX_CHANNELS) {
            CmVec3 av = motion_get_angular_velocity(&g_motion_sys.channels[ch]);
            return float_to_word(av.x);
        }
#endif
        return float_to_word(0);
    }

    case MOT_CALL_GET_ANGULAR_VEL_Y: {
#ifdef CALYNDA_WII_BUILD
        int ch = (argument_count > 0) ? (int)arguments[0] : 0;
        if (g_motion_inited && ch >= 0 && ch < MOTION_MAX_CHANNELS) {
            CmVec3 av = motion_get_angular_velocity(&g_motion_sys.channels[ch]);
            return float_to_word(av.y);
        }
#endif
        return float_to_word(0);
    }

    case MOT_CALL_GET_ANGULAR_VEL_Z: {
#ifdef CALYNDA_WII_BUILD
        int ch = (argument_count > 0) ? (int)arguments[0] : 0;
        if (g_motion_inited && ch >= 0 && ch < MOTION_MAX_CHANNELS) {
            CmVec3 av = motion_get_angular_velocity(&g_motion_sys.channels[ch]);
            return float_to_word(av.z);
        }
#endif
        return float_to_word(0);
    }

    case MOT_CALL_GET_LINEAR_ACCEL_X: {
#ifdef CALYNDA_WII_BUILD
        int ch = (argument_count > 0) ? (int)arguments[0] : 0;
        if (g_motion_inited && ch >= 0 && ch < MOTION_MAX_CHANNELS) {
            CmVec3 la = motion_get_linear_accel(&g_motion_sys.channels[ch]);
            return float_to_word(la.x);
        }
#endif
        return float_to_word(0);
    }

    case MOT_CALL_GET_LINEAR_ACCEL_Y: {
#ifdef CALYNDA_WII_BUILD
        int ch = (argument_count > 0) ? (int)arguments[0] : 0;
        if (g_motion_inited && ch >= 0 && ch < MOTION_MAX_CHANNELS) {
            CmVec3 la = motion_get_linear_accel(&g_motion_sys.channels[ch]);
            return float_to_word(la.y);
        }
#endif
        return float_to_word(0);
    }

    case MOT_CALL_GET_LINEAR_ACCEL_Z: {
#ifdef CALYNDA_WII_BUILD
        int ch = (argument_count > 0) ? (int)arguments[0] : 0;
        if (g_motion_inited && ch >= 0 && ch < MOTION_MAX_CHANNELS) {
            CmVec3 la = motion_get_linear_accel(&g_motion_sys.channels[ch]);
            return float_to_word(la.z);
        }
#endif
        return float_to_word(0);
    }

    case MOT_CALL_GET_GESTURE: {
#ifdef CALYNDA_WII_BUILD
        int ch = (argument_count > 0) ? (int)arguments[0] : 0;
        if (g_motion_inited && ch >= 0 && ch < MOTION_MAX_CHANNELS) {
            MotionGesture g = motion_get_gesture(&g_motion_sys.channels[ch]);
            return (CalyndaRtWord)g.type;
        }
#endif
        return 0;
    }

    case MOT_CALL_GET_GESTURE_STRENGTH: {
#ifdef CALYNDA_WII_BUILD
        int ch = (argument_count > 0) ? (int)arguments[0] : 0;
        if (g_motion_inited && ch >= 0 && ch < MOTION_MAX_CHANNELS) {
            MotionGesture g = motion_get_gesture(&g_motion_sys.channels[ch]);
            return float_to_word(g.intensity);
        }
#endif
        return float_to_word(0);
    }

    case MOT_CALL_GET_POINTER_X: {
#ifdef CALYNDA_WII_BUILD
        int ch = (argument_count > 0) ? (int)arguments[0] : 0;
        if (g_motion_inited && ch >= 0 && ch < MOTION_MAX_CHANNELS) {
            float x, y; bool vis;
            motion_get_pointer(&g_motion_sys.channels[ch], &x, &y, &vis);
            return float_to_word(x);
        }
#endif
        return float_to_word(0);
    }

    case MOT_CALL_GET_POINTER_Y: {
#ifdef CALYNDA_WII_BUILD
        int ch = (argument_count > 0) ? (int)arguments[0] : 0;
        if (g_motion_inited && ch >= 0 && ch < MOTION_MAX_CHANNELS) {
            float x, y; bool vis;
            motion_get_pointer(&g_motion_sys.channels[ch], &x, &y, &vis);
            return float_to_word(y);
        }
#endif
        return float_to_word(0);
    }

    case MOT_CALL_GET_AIM_RAY_OX: {
#ifdef CALYNDA_WII_BUILD
        int ch = (argument_count > 0) ? (int)arguments[0] : 0;
        if (g_motion_inited && ch >= 0 && ch < MOTION_MAX_CHANNELS) {
            CmRay r = motion_get_aim_ray(&g_motion_sys.channels[ch]);
            return float_to_word(r.origin.x);
        }
#endif
        return float_to_word(0);
    }

    case MOT_CALL_GET_AIM_RAY_OY: {
#ifdef CALYNDA_WII_BUILD
        int ch = (argument_count > 0) ? (int)arguments[0] : 0;
        if (g_motion_inited && ch >= 0 && ch < MOTION_MAX_CHANNELS) {
            CmRay r = motion_get_aim_ray(&g_motion_sys.channels[ch]);
            return float_to_word(r.origin.y);
        }
#endif
        return float_to_word(0);
    }

    case MOT_CALL_GET_AIM_RAY_OZ: {
#ifdef CALYNDA_WII_BUILD
        int ch = (argument_count > 0) ? (int)arguments[0] : 0;
        if (g_motion_inited && ch >= 0 && ch < MOTION_MAX_CHANNELS) {
            CmRay r = motion_get_aim_ray(&g_motion_sys.channels[ch]);
            return float_to_word(r.origin.z);
        }
#endif
        return float_to_word(0);
    }

    case MOT_CALL_GET_AIM_RAY_DX: {
#ifdef CALYNDA_WII_BUILD
        int ch = (argument_count > 0) ? (int)arguments[0] : 0;
        if (g_motion_inited && ch >= 0 && ch < MOTION_MAX_CHANNELS) {
            CmRay r = motion_get_aim_ray(&g_motion_sys.channels[ch]);
            return float_to_word(r.direction.x);
        }
#endif
        return float_to_word(0);
    }

    case MOT_CALL_GET_AIM_RAY_DY: {
#ifdef CALYNDA_WII_BUILD
        int ch = (argument_count > 0) ? (int)arguments[0] : 0;
        if (g_motion_inited && ch >= 0 && ch < MOTION_MAX_CHANNELS) {
            CmRay r = motion_get_aim_ray(&g_motion_sys.channels[ch]);
            return float_to_word(r.direction.y);
        }
#endif
        return float_to_word(0);
    }

    case MOT_CALL_GET_AIM_RAY_DZ: {
#ifdef CALYNDA_WII_BUILD
        int ch = (argument_count > 0) ? (int)arguments[0] : 0;
        if (g_motion_inited && ch >= 0 && ch < MOTION_MAX_CHANNELS) {
            CmRay r = motion_get_aim_ray(&g_motion_sys.channels[ch]);
            return float_to_word(r.direction.z);
        }
#endif
        return float_to_word(0);
    }

    /* ---- Config ---- */
    case MOT_CALL_SET_THRESHOLDS:
#ifdef CALYNDA_WII_BUILD
        /* args: channel, swing_thresh, thrust_thresh, twist_thresh */
        if (g_motion_inited && argument_count >= 4) {
            int ch = (int)arguments[0];
            if (ch >= 0 && ch < MOTION_MAX_CHANNELS) {
                motion_set_swing_threshold(&g_motion_sys.channels[ch],
                                            word_to_float(arguments[1]));
                motion_set_thrust_threshold(&g_motion_sys.channels[ch],
                                             word_to_float(arguments[2]));
                motion_set_twist_threshold(&g_motion_sys.channels[ch],
                                            word_to_float(arguments[3]));
            }
        }
#else
        fprintf(stderr, "[motion.set_thresholds] stub\n");
#endif
        return 0;

    case MOT_CALL_CALIBRATE:
#ifdef CALYNDA_WII_BUILD
        {
            int ch = (argument_count > 0) ? (int)arguments[0] : 0;
            if (g_motion_inited && ch >= 0 && ch < MOTION_MAX_CHANNELS)
                motion_calibrate(&g_motion_sys.channels[ch]);
        }
#else
        fprintf(stderr, "[motion.calibrate] stub\n");
#endif
        return 0;

    case MOT_CALL_RESET_ORIENTATION:
#ifdef CALYNDA_WII_BUILD
        {
            int ch = (argument_count > 0) ? (int)arguments[0] : 0;
            if (g_motion_inited && ch >= 0 && ch < MOTION_MAX_CHANNELS)
                motion_reset_orientation(&g_motion_sys.channels[ch]);
        }
#else
        fprintf(stderr, "[motion.reset_orientation] stub\n");
#endif
        return 0;

    /* ---- Gesture type constants → integer IDs ---- */
    case MOT_CALL_GESTURE_NONE:        return 0;
    case MOT_CALL_GESTURE_SWING_LEFT:  return 1;
    case MOT_CALL_GESTURE_SWING_RIGHT: return 2;
    case MOT_CALL_GESTURE_SWING_UP:    return 3;
    case MOT_CALL_GESTURE_SWING_DOWN:  return 4;
    case MOT_CALL_GESTURE_THRUST:      return 5;
    case MOT_CALL_GESTURE_THROW:       return 6;
    case MOT_CALL_GESTURE_TWIST_CW:    return 7;
    case MOT_CALL_GESTURE_TWIST_CCW:   return 8;
    case MOT_CALL_GESTURE_SHAKE:       return 9;
    }

    fprintf(stderr, "runtime: unsupported motion callable kind %d\n",
            (int)callable->kind);
    return 0;
}

/* ------------------------------------------------------------------ */
/* Member Load                                                          */
/* ------------------------------------------------------------------ */

static CalyndaRtWord rt_make_obj(void *p) {
    return (CalyndaRtWord)(uintptr_t)p;
}

CalyndaRtWord calynda_motion_member_load(const char *member) {
    if (!member) return 0;

    if (strcmp(member, "init")               == 0) return rt_make_obj(&MOT_INIT_CALLABLE);
    if (strcmp(member, "update")             == 0) return rt_make_obj(&MOT_UPDATE_CALLABLE);
    if (strcmp(member, "feed_raw")           == 0) return rt_make_obj(&MOT_FEED_RAW_CALLABLE);
    if (strcmp(member, "feed_ir")            == 0) return rt_make_obj(&MOT_FEED_IR_CALLABLE);
    if (strcmp(member, "get_pitch")          == 0) return rt_make_obj(&MOT_PITCH_CALLABLE);
    if (strcmp(member, "get_yaw")            == 0) return rt_make_obj(&MOT_YAW_CALLABLE);
    if (strcmp(member, "get_roll")           == 0) return rt_make_obj(&MOT_ROLL_CALLABLE);
    if (strcmp(member, "get_angular_vel_x")  == 0) return rt_make_obj(&MOT_ANG_X_CALLABLE);
    if (strcmp(member, "get_angular_vel_y")  == 0) return rt_make_obj(&MOT_ANG_Y_CALLABLE);
    if (strcmp(member, "get_angular_vel_z")  == 0) return rt_make_obj(&MOT_ANG_Z_CALLABLE);
    if (strcmp(member, "get_linear_accel_x") == 0) return rt_make_obj(&MOT_LIN_X_CALLABLE);
    if (strcmp(member, "get_linear_accel_y") == 0) return rt_make_obj(&MOT_LIN_Y_CALLABLE);
    if (strcmp(member, "get_linear_accel_z") == 0) return rt_make_obj(&MOT_LIN_Z_CALLABLE);
    if (strcmp(member, "get_gesture")        == 0) return rt_make_obj(&MOT_GESTURE_CALLABLE);
    if (strcmp(member, "get_gesture_strength") == 0) return rt_make_obj(&MOT_GESTURE_STR_CALLABLE);
    if (strcmp(member, "get_pointer_x")     == 0) return rt_make_obj(&MOT_PTR_X_CALLABLE);
    if (strcmp(member, "get_pointer_y")     == 0) return rt_make_obj(&MOT_PTR_Y_CALLABLE);
    if (strcmp(member, "get_aim_ray_ox")    == 0) return rt_make_obj(&MOT_AIM_OX_CALLABLE);
    if (strcmp(member, "get_aim_ray_oy")    == 0) return rt_make_obj(&MOT_AIM_OY_CALLABLE);
    if (strcmp(member, "get_aim_ray_oz")    == 0) return rt_make_obj(&MOT_AIM_OZ_CALLABLE);
    if (strcmp(member, "get_aim_ray_dx")    == 0) return rt_make_obj(&MOT_AIM_DX_CALLABLE);
    if (strcmp(member, "get_aim_ray_dy")    == 0) return rt_make_obj(&MOT_AIM_DY_CALLABLE);
    if (strcmp(member, "get_aim_ray_dz")    == 0) return rt_make_obj(&MOT_AIM_DZ_CALLABLE);
    if (strcmp(member, "set_thresholds")    == 0) return rt_make_obj(&MOT_THRESHOLDS_CALLABLE);
    if (strcmp(member, "calibrate")         == 0) return rt_make_obj(&MOT_CALIBRATE_CALLABLE);
    if (strcmp(member, "reset_orientation") == 0) return rt_make_obj(&MOT_RESET_ORI_CALLABLE);
    if (strcmp(member, "GESTURE_NONE")        == 0) return rt_make_obj(&MOT_G_NONE_CALLABLE);
    if (strcmp(member, "GESTURE_SWING_LEFT")  == 0) return rt_make_obj(&MOT_G_SWING_L_CALLABLE);
    if (strcmp(member, "GESTURE_SWING_RIGHT") == 0) return rt_make_obj(&MOT_G_SWING_R_CALLABLE);
    if (strcmp(member, "GESTURE_SWING_UP")    == 0) return rt_make_obj(&MOT_G_SWING_U_CALLABLE);
    if (strcmp(member, "GESTURE_SWING_DOWN")  == 0) return rt_make_obj(&MOT_G_SWING_D_CALLABLE);
    if (strcmp(member, "GESTURE_THRUST")      == 0) return rt_make_obj(&MOT_G_THRUST_CALLABLE);
    if (strcmp(member, "GESTURE_THROW")       == 0) return rt_make_obj(&MOT_G_THROW_CALLABLE);
    if (strcmp(member, "GESTURE_TWIST_CW")    == 0) return rt_make_obj(&MOT_G_TWIST_CW_CALLABLE);
    if (strcmp(member, "GESTURE_TWIST_CCW")   == 0) return rt_make_obj(&MOT_G_TWIST_CCW_CALLABLE);
    if (strcmp(member, "GESTURE_SHAKE")       == 0) return rt_make_obj(&MOT_G_SHAKE_CALLABLE);

    fprintf(stderr, "runtime: motion has no member '%s'\n", member);
    return 0;
}
