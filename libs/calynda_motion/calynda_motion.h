/*
 * calynda_motion.h — MotionPlus, gesture recognition, and IR pointer
 * for Calynda Wii.
 *
 * Features:
 * - MotionPlus gyroscope + accelerometer fusion (complementary filter)
 * - Gesture recognition: swing, thrust, throw, twist
 * - Smoothed IR pointer with aim ray projection
 */

#ifndef CALYNDA_MOTION_H
#define CALYNDA_MOTION_H

#include "../calynda_math/calynda_math.h"

#include <stdbool.h>
#include <stdint.h>

/* ================================================================== */
/* Configuration                                                        */
/* ================================================================== */

#define MOTION_MAX_CHANNELS      4     /* one per Wii Remote */
#define MOTION_GESTURE_HISTORY  30     /* frames of sample history */
#define MOTION_IR_SMOOTH_FACTOR  0.15f /* exponential smoothing */

/* ================================================================== */
/* Accelerometer + Gyroscope Raw Data                                   */
/* ================================================================== */

typedef struct {
    CmVec3  accel;       /* accelerometer in g (1g = 9.81 m/s^2) */
    CmVec3  gyro;        /* gyroscope in deg/s (MotionPlus) */
    bool    mp_connected; /* MotionPlus present */
} MotionRawInput;

/* ================================================================== */
/* Orientation (fused)                                                  */
/* ================================================================== */

typedef struct {
    CmQuat  orientation;          /* current fused orientation */
    CmVec3  angular_velocity;     /* deg/s, filtered */
    CmVec3  linear_accel;         /* acceleration minus gravity estimate */
    CmVec3  gravity_estimate;     /* estimated gravity direction in sensor frame */
    float   complementary_alpha;  /* filter blend (0..1), default 0.98 */
} MotionOrientation;

/* ================================================================== */
/* Gesture Types                                                        */
/* ================================================================== */

typedef enum {
    GESTURE_NONE,
    GESTURE_SWING_LEFT,
    GESTURE_SWING_RIGHT,
    GESTURE_SWING_UP,
    GESTURE_SWING_DOWN,
    GESTURE_THRUST,       /* forward stab */
    GESTURE_THROW,        /* overhand throw */
    GESTURE_TWIST_CW,     /* clockwise twist */
    GESTURE_TWIST_CCW,    /* counter-clockwise twist */
    GESTURE_SHAKE,        /* rapid back and forth */

    GESTURE_COUNT
} MotionGestureType;

typedef struct {
    MotionGestureType type;
    float  intensity;      /* 0..1, normalized magnitude */
    CmVec3 direction;      /* primary axis of the gesture */
    float  duration;       /* seconds the gesture lasted */
    bool   active;         /* true while gesture is ongoing */
} MotionGesture;

/* ================================================================== */
/* Gesture Detector State                                               */
/* ================================================================== */

typedef struct {
    CmVec3 accel_history[MOTION_GESTURE_HISTORY];
    CmVec3 gyro_history[MOTION_GESTURE_HISTORY];
    int    history_index;
    int    history_count;

    /* Detection thresholds */
    float  swing_threshold;     /* accel magnitude threshold for swing */
    float  thrust_threshold;    /* forward accel threshold */
    float  twist_threshold;     /* gyro Z threshold for twist */
    float  shake_threshold;     /* accel variance for shake */
    float  cooldown_timer;      /* prevent rapid-fire detections */
    float  cooldown_duration;   /* seconds */

    MotionGesture last_gesture;
} MotionGestureDetector;

/* ================================================================== */
/* IR Pointer                                                           */
/* ================================================================== */

typedef struct {
    float screen_x;     /* smoothed screen position (0..1) */
    float screen_y;
    float raw_x;        /* unsmoothed raw */
    float raw_y;
    bool  visible;      /* any IR dots seen */

    /* Projected aim ray in 3D space */
    CmRay aim_ray;      /* ray from camera into scene */
} MotionIRPointer;

/* ================================================================== */
/* Motion Channel (per remote)                                          */
/* ================================================================== */

typedef struct {
    int                   channel;   /* 0..3 */
    bool                  active;

    MotionRawInput        raw;
    MotionOrientation     orient;
    MotionGestureDetector gesture_det;
    MotionIRPointer       ir;
} MotionChannel;

/* ================================================================== */
/* Motion System                                                        */
/* ================================================================== */

typedef struct {
    MotionChannel channels[MOTION_MAX_CHANNELS];
    int           active_count;
} MotionSystem;

/* ================================================================== */
/* API                                                                  */
/* ================================================================== */

/* Initialize the motion system */
void motion_init(MotionSystem *sys);

/* Update one channel with new raw data. Call once per frame per remote. */
void motion_update(MotionChannel *ch, float dt);

/* Feed raw input (from WPAD extension data) */
void motion_feed_raw(MotionChannel *ch, CmVec3 accel, CmVec3 gyro, bool mp_connected);

/* Feed IR data from WPAD */
void motion_feed_ir(MotionChannel *ch, float raw_x, float raw_y, bool visible);

/* Get current orientation */
CmQuat motion_get_orientation(const MotionChannel *ch);

/* Get angular velocity (deg/s) */
CmVec3 motion_get_angular_velocity(const MotionChannel *ch);

/* Get linear acceleration (gravity removed) */
CmVec3 motion_get_linear_accel(const MotionChannel *ch);

/* Get latest detected gesture (or GESTURE_NONE) */
MotionGesture motion_get_gesture(const MotionChannel *ch);

/* Get smoothed IR pointer position (0..1 screen coords) */
void motion_get_pointer(const MotionChannel *ch, float *x, float *y, bool *visible);

/* Get aim ray projected from IR pointer into 3D scene */
CmRay motion_get_aim_ray(const MotionChannel *ch);

/* Set gesture detection thresholds */
void motion_set_swing_threshold(MotionChannel *ch, float threshold);
void motion_set_thrust_threshold(MotionChannel *ch, float threshold);
void motion_set_twist_threshold(MotionChannel *ch, float threshold);

/* Calibrate: call while remote is held still, facing forward */
void motion_calibrate(MotionChannel *ch);

/* Reset orientation to identity */
void motion_reset_orientation(MotionChannel *ch);

#endif /* CALYNDA_MOTION_H */
