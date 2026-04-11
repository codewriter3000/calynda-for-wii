/*
 * calynda_math.h — 3D math primitives for Calynda Wii.
 *
 * Float32 types matching GX hardware: Vec3, Vec4, Quat, Mat4,
 * plus collision geometry (AABB, Sphere, Ray, Plane) with
 * intersection tests. Thin wrappers around libogc guMtx / guVec
 * where appropriate.
 */

#ifndef CALYNDA_MATH_H
#define CALYNDA_MATH_H

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#ifdef CALYNDA_WII_BUILD
#include <ogc/gu.h>
#include <ogc/gx.h>
#endif

/* ================================================================== */
/* Constants                                                            */
/* ================================================================== */

#define CM_PI       3.14159265358979323846f
#define CM_DEG2RAD  (CM_PI / 180.0f)
#define CM_RAD2DEG  (180.0f / CM_PI)
#define CM_EPSILON  1e-6f

/* ================================================================== */
/* Vec3                                                                 */
/* ================================================================== */

typedef struct {
    float x, y, z;
} CmVec3;

static inline CmVec3 cm_vec3(float x, float y, float z) {
    return (CmVec3){ x, y, z };
}

static inline CmVec3 cm_vec3_zero(void) {
    return (CmVec3){ 0.0f, 0.0f, 0.0f };
}

static inline CmVec3 cm_vec3_one(void) {
    return (CmVec3){ 1.0f, 1.0f, 1.0f };
}

static inline CmVec3 cm_vec3_up(void) {
    return (CmVec3){ 0.0f, 1.0f, 0.0f };
}

static inline CmVec3 cm_vec3_add(CmVec3 a, CmVec3 b) {
    return (CmVec3){ a.x + b.x, a.y + b.y, a.z + b.z };
}

static inline CmVec3 cm_vec3_sub(CmVec3 a, CmVec3 b) {
    return (CmVec3){ a.x - b.x, a.y - b.y, a.z - b.z };
}

static inline CmVec3 cm_vec3_mul(CmVec3 a, CmVec3 b) {
    return (CmVec3){ a.x * b.x, a.y * b.y, a.z * b.z };
}

static inline CmVec3 cm_vec3_scale(CmVec3 v, float s) {
    return (CmVec3){ v.x * s, v.y * s, v.z * s };
}

static inline CmVec3 cm_vec3_neg(CmVec3 v) {
    return (CmVec3){ -v.x, -v.y, -v.z };
}

static inline float cm_vec3_dot(CmVec3 a, CmVec3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

static inline CmVec3 cm_vec3_cross(CmVec3 a, CmVec3 b) {
    return (CmVec3){
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
}

static inline float cm_vec3_length_sq(CmVec3 v) {
    return cm_vec3_dot(v, v);
}

static inline float cm_vec3_length(CmVec3 v) {
    return sqrtf(cm_vec3_length_sq(v));
}

static inline CmVec3 cm_vec3_normalize(CmVec3 v) {
    float len = cm_vec3_length(v);
    if (len < CM_EPSILON) return cm_vec3_zero();
    return cm_vec3_scale(v, 1.0f / len);
}

static inline float cm_vec3_distance(CmVec3 a, CmVec3 b) {
    return cm_vec3_length(cm_vec3_sub(a, b));
}

static inline CmVec3 cm_vec3_lerp(CmVec3 a, CmVec3 b, float t) {
    return cm_vec3_add(cm_vec3_scale(a, 1.0f - t), cm_vec3_scale(b, t));
}

static inline CmVec3 cm_vec3_min(CmVec3 a, CmVec3 b) {
    return (CmVec3){
        a.x < b.x ? a.x : b.x,
        a.y < b.y ? a.y : b.y,
        a.z < b.z ? a.z : b.z
    };
}

static inline CmVec3 cm_vec3_max(CmVec3 a, CmVec3 b) {
    return (CmVec3){
        a.x > b.x ? a.x : b.x,
        a.y > b.y ? a.y : b.y,
        a.z > b.z ? a.z : b.z
    };
}

static inline CmVec3 cm_vec3_reflect(CmVec3 v, CmVec3 n) {
    return cm_vec3_sub(v, cm_vec3_scale(n, 2.0f * cm_vec3_dot(v, n)));
}

/* ================================================================== */
/* Vec4                                                                 */
/* ================================================================== */

typedef struct {
    float x, y, z, w;
} CmVec4;

static inline CmVec4 cm_vec4(float x, float y, float z, float w) {
    return (CmVec4){ x, y, z, w };
}

static inline CmVec4 cm_vec4_from_vec3(CmVec3 v, float w) {
    return (CmVec4){ v.x, v.y, v.z, w };
}

static inline CmVec3 cm_vec4_xyz(CmVec4 v) {
    return (CmVec3){ v.x, v.y, v.z };
}

static inline float cm_vec4_dot(CmVec4 a, CmVec4 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

/* ================================================================== */
/* Quaternion                                                           */
/* ================================================================== */

typedef struct {
    float x, y, z, w;   /* (x,y,z) = imaginary, w = real */
} CmQuat;

static inline CmQuat cm_quat(float x, float y, float z, float w) {
    return (CmQuat){ x, y, z, w };
}

static inline CmQuat cm_quat_identity(void) {
    return (CmQuat){ 0.0f, 0.0f, 0.0f, 1.0f };
}

static inline float cm_quat_length_sq(CmQuat q) {
    return q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w;
}

static inline float cm_quat_length(CmQuat q) {
    return sqrtf(cm_quat_length_sq(q));
}

static inline CmQuat cm_quat_normalize(CmQuat q) {
    float len = cm_quat_length(q);
    if (len < CM_EPSILON) return cm_quat_identity();
    float inv = 1.0f / len;
    return (CmQuat){ q.x * inv, q.y * inv, q.z * inv, q.w * inv };
}

static inline CmQuat cm_quat_conjugate(CmQuat q) {
    return (CmQuat){ -q.x, -q.y, -q.z, q.w };
}

static inline CmQuat cm_quat_inverse(CmQuat q) {
    float len_sq = cm_quat_length_sq(q);
    if (len_sq < CM_EPSILON) return cm_quat_identity();
    float inv = 1.0f / len_sq;
    return (CmQuat){ -q.x * inv, -q.y * inv, -q.z * inv, q.w * inv };
}

static inline CmQuat cm_quat_mul(CmQuat a, CmQuat b) {
    return (CmQuat){
        a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y,
        a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x,
        a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w,
        a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z
    };
}

static inline CmVec3 cm_quat_rotate_vec3(CmQuat q, CmVec3 v) {
    /* q * (v,0) * q^-1  — expanded for efficiency */
    CmVec3 u = { q.x, q.y, q.z };
    float s = q.w;
    CmVec3 uv = cm_vec3_cross(u, v);
    CmVec3 uuv = cm_vec3_cross(u, uv);
    return cm_vec3_add(v, cm_vec3_scale(cm_vec3_add(cm_vec3_scale(uv, s), uuv), 2.0f));
}

static inline CmQuat cm_quat_from_axis_angle(CmVec3 axis, float angle_rad) {
    float half = angle_rad * 0.5f;
    float s = sinf(half);
    CmVec3 n = cm_vec3_normalize(axis);
    return (CmQuat){ n.x * s, n.y * s, n.z * s, cosf(half) };
}

static inline CmQuat cm_quat_from_euler(float pitch, float yaw, float roll) {
    float cp = cosf(pitch * 0.5f), sp = sinf(pitch * 0.5f);
    float cy = cosf(yaw * 0.5f),   sy = sinf(yaw * 0.5f);
    float cr = cosf(roll * 0.5f),  sr = sinf(roll * 0.5f);
    return (CmQuat){
        sr * cp * cy - cr * sp * sy,
        cr * sp * cy + sr * cp * sy,
        cr * cp * sy - sr * sp * cy,
        cr * cp * cy + sr * sp * sy
    };
}

static inline float cm_quat_dot(CmQuat a, CmQuat b) {
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

/* Spherical linear interpolation */
static inline CmQuat cm_quat_slerp(CmQuat a, CmQuat b, float t) {
    float dot = cm_quat_dot(a, b);
    if (dot < 0.0f) {
        b = (CmQuat){ -b.x, -b.y, -b.z, -b.w };
        dot = -dot;
    }
    if (dot > 0.9995f) {
        /* Linear interpolation for very close quaternions */
        CmQuat r = {
            a.x + t * (b.x - a.x),
            a.y + t * (b.y - a.y),
            a.z + t * (b.z - a.z),
            a.w + t * (b.w - a.w)
        };
        return cm_quat_normalize(r);
    }
    float theta = acosf(dot);
    float sin_theta = sinf(theta);
    float wa = sinf((1.0f - t) * theta) / sin_theta;
    float wb = sinf(t * theta) / sin_theta;
    return (CmQuat){
        wa * a.x + wb * b.x,
        wa * a.y + wb * b.y,
        wa * a.z + wb * b.z,
        wa * a.w + wb * b.w
    };
}

/* ================================================================== */
/* Mat4 — column-major 4x4 matrix                                      */
/* ================================================================== */

typedef struct {
    float m[16];   /* column-major: m[col*4 + row] */
} CmMat4;

#define CM_MAT4_AT(mat, row, col) ((mat).m[(col) * 4 + (row)])

static inline CmMat4 cm_mat4_identity(void) {
    CmMat4 r;
    memset(&r, 0, sizeof(r));
    r.m[0] = r.m[5] = r.m[10] = r.m[15] = 1.0f;
    return r;
}

static inline CmMat4 cm_mat4_zero(void) {
    CmMat4 r;
    memset(&r, 0, sizeof(r));
    return r;
}

static inline CmMat4 cm_mat4_mul(CmMat4 a, CmMat4 b) {
    CmMat4 r;
    for (int c = 0; c < 4; c++) {
        for (int row = 0; row < 4; row++) {
            float sum = 0.0f;
            for (int k = 0; k < 4; k++) {
                sum += a.m[k * 4 + row] * b.m[c * 4 + k];
            }
            r.m[c * 4 + row] = sum;
        }
    }
    return r;
}

static inline CmVec4 cm_mat4_mul_vec4(CmMat4 m, CmVec4 v) {
    return (CmVec4){
        m.m[0]*v.x + m.m[4]*v.y + m.m[8]*v.z  + m.m[12]*v.w,
        m.m[1]*v.x + m.m[5]*v.y + m.m[9]*v.z  + m.m[13]*v.w,
        m.m[2]*v.x + m.m[6]*v.y + m.m[10]*v.z + m.m[14]*v.w,
        m.m[3]*v.x + m.m[7]*v.y + m.m[11]*v.z + m.m[15]*v.w
    };
}

static inline CmVec3 cm_mat4_mul_point(CmMat4 m, CmVec3 p) {
    CmVec4 r = cm_mat4_mul_vec4(m, cm_vec4_from_vec3(p, 1.0f));
    return cm_vec4_xyz(r);
}

static inline CmVec3 cm_mat4_mul_dir(CmMat4 m, CmVec3 d) {
    CmVec4 r = cm_mat4_mul_vec4(m, cm_vec4_from_vec3(d, 0.0f));
    return cm_vec4_xyz(r);
}

static inline CmMat4 cm_mat4_translate(CmVec3 t) {
    CmMat4 r = cm_mat4_identity();
    r.m[12] = t.x; r.m[13] = t.y; r.m[14] = t.z;
    return r;
}

static inline CmMat4 cm_mat4_scale(CmVec3 s) {
    CmMat4 r = cm_mat4_zero();
    r.m[0] = s.x; r.m[5] = s.y; r.m[10] = s.z; r.m[15] = 1.0f;
    return r;
}

static inline CmMat4 cm_mat4_rotate_x(float angle_rad) {
    float c = cosf(angle_rad), s = sinf(angle_rad);
    CmMat4 r = cm_mat4_identity();
    r.m[5] = c;  r.m[9]  = -s;
    r.m[6] = s;  r.m[10] =  c;
    return r;
}

static inline CmMat4 cm_mat4_rotate_y(float angle_rad) {
    float c = cosf(angle_rad), s = sinf(angle_rad);
    CmMat4 r = cm_mat4_identity();
    r.m[0] =  c;  r.m[8]  = s;
    r.m[2] = -s;  r.m[10] = c;
    return r;
}

static inline CmMat4 cm_mat4_rotate_z(float angle_rad) {
    float c = cosf(angle_rad), s = sinf(angle_rad);
    CmMat4 r = cm_mat4_identity();
    r.m[0] = c;  r.m[4] = -s;
    r.m[1] = s;  r.m[5] =  c;
    return r;
}

static inline CmMat4 cm_mat4_from_quat(CmQuat q) {
    float xx = q.x * q.x, yy = q.y * q.y, zz = q.z * q.z;
    float xy = q.x * q.y, xz = q.x * q.z, yz = q.y * q.z;
    float wx = q.w * q.x, wy = q.w * q.y, wz = q.w * q.z;
    CmMat4 r = cm_mat4_identity();
    r.m[0] = 1.0f - 2.0f*(yy + zz);  r.m[4] = 2.0f*(xy - wz);        r.m[8]  = 2.0f*(xz + wy);
    r.m[1] = 2.0f*(xy + wz);          r.m[5] = 1.0f - 2.0f*(xx + zz); r.m[9]  = 2.0f*(yz - wx);
    r.m[2] = 2.0f*(xz - wy);          r.m[6] = 2.0f*(yz + wx);        r.m[10] = 1.0f - 2.0f*(xx + yy);
    return r;
}

static inline CmMat4 cm_mat4_trs(CmVec3 pos, CmQuat rot, CmVec3 scl) {
    CmMat4 r = cm_mat4_from_quat(rot);
    /* Apply scale to rotation columns */
    r.m[0] *= scl.x; r.m[1] *= scl.x; r.m[2] *= scl.x;
    r.m[4] *= scl.y; r.m[5] *= scl.y; r.m[6] *= scl.y;
    r.m[8] *= scl.z; r.m[9] *= scl.z; r.m[10] *= scl.z;
    /* Set translation */
    r.m[12] = pos.x; r.m[13] = pos.y; r.m[14] = pos.z;
    return r;
}

static inline CmMat4 cm_mat4_transpose(CmMat4 m) {
    CmMat4 r;
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            r.m[j * 4 + i] = m.m[i * 4 + j];
    return r;
}

/* ------------------------------------------------------------------ */
/* Projection & View (wraps libogc on Wii, standalone on host)         */
/* ------------------------------------------------------------------ */

CmMat4 cm_perspective(float fov_y_deg, float aspect, float near_z, float far_z);
CmMat4 cm_lookat(CmVec3 eye, CmVec3 target, CmVec3 up);
CmMat4 cm_ortho(float left, float right, float bottom, float top, float near_z, float far_z);

/* ================================================================== */
/* Collision Geometry Primitives                                        */
/* ================================================================== */

typedef struct {
    CmVec3 min;
    CmVec3 max;
} CmAABB;

typedef struct {
    CmVec3 center;
    float  radius;
} CmSphere;

typedef struct {
    CmVec3 origin;
    CmVec3 direction;   /* should be normalized */
} CmRay;

typedef struct {
    CmVec3 normal;      /* should be normalized */
    float  distance;    /* signed distance from origin: dot(normal, point) = distance */
} CmPlane;

typedef struct {
    CmVec3 a, b;       /* endpoints */
    float  radius;
} CmCapsule;

/* Contact information returned by intersection tests */
typedef struct {
    CmVec3 point;
    CmVec3 normal;
    float  depth;       /* penetration depth (>0 means overlapping) */
    bool   hit;
} CmContact;

/* ================================================================== */
/* AABB helpers                                                         */
/* ================================================================== */

static inline CmAABB cm_aabb_from_center_half(CmVec3 center, CmVec3 half_extents) {
    return (CmAABB){
        cm_vec3_sub(center, half_extents),
        cm_vec3_add(center, half_extents)
    };
}

static inline CmVec3 cm_aabb_center(CmAABB b) {
    return cm_vec3_scale(cm_vec3_add(b.min, b.max), 0.5f);
}

static inline CmVec3 cm_aabb_half_extents(CmAABB b) {
    return cm_vec3_scale(cm_vec3_sub(b.max, b.min), 0.5f);
}

static inline CmAABB cm_aabb_merge(CmAABB a, CmAABB b) {
    return (CmAABB){
        cm_vec3_min(a.min, b.min),
        cm_vec3_max(a.max, b.max)
    };
}

static inline bool cm_aabb_contains_point(CmAABB b, CmVec3 p) {
    return p.x >= b.min.x && p.x <= b.max.x &&
           p.y >= b.min.y && p.y <= b.max.y &&
           p.z >= b.min.z && p.z <= b.max.z;
}

/* ================================================================== */
/* Intersection Tests (declarations — implemented in calynda_math.c)    */
/* ================================================================== */

bool cm_sphere_vs_sphere(CmSphere a, CmSphere b, CmContact *out);
bool cm_sphere_vs_aabb(CmSphere s, CmAABB b, CmContact *out);
bool cm_sphere_vs_plane(CmSphere s, CmPlane p, CmContact *out);
bool cm_aabb_vs_aabb(CmAABB a, CmAABB b, CmContact *out);
bool cm_capsule_vs_capsule(CmCapsule a, CmCapsule b, CmContact *out);

/* Ray casts — returns true if hit, t = distance along ray */
bool cm_ray_vs_sphere(CmRay ray, CmSphere s, float *t);
bool cm_ray_vs_plane(CmRay ray, CmPlane p, float *t);
bool cm_ray_vs_aabb(CmRay ray, CmAABB b, float *t);
bool cm_ray_vs_triangle(CmRay ray, CmVec3 v0, CmVec3 v1, CmVec3 v2, float *t);

/* ================================================================== */
/* Frustum (6 planes — for culling)                                     */
/* ================================================================== */

typedef struct {
    CmPlane planes[6];   /* left, right, bottom, top, near, far */
} CmFrustum;

CmFrustum cm_frustum_from_vp(CmMat4 view_proj);
bool cm_frustum_contains_sphere(const CmFrustum *f, CmSphere s);
bool cm_frustum_contains_aabb(const CmFrustum *f, CmAABB b);

/* ================================================================== */
/* Utility                                                              */
/* ================================================================== */

static inline float cm_clampf(float x, float lo, float hi) {
    if (x < lo) return lo;
    if (x > hi) return hi;
    return x;
}

static inline float cm_lerpf(float a, float b, float t) {
    return a + (b - a) * t;
}

static inline float cm_minf(float a, float b) { return a < b ? a : b; }
static inline float cm_maxf(float a, float b) { return a > b ? a : b; }
static inline float cm_absf(float a) { return a < 0.0f ? -a : a; }

#endif /* CALYNDA_MATH_H */
