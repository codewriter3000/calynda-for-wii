/*
 * test_math.c — Unit tests for calynda_math (Vec3, Vec4, Quat, Mat4,
 * collision geometry, intersection tests, frustum culling).
 */

#include "calynda_math.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

#define ASSERT_TRUE(condition, msg) do {                                    \
    tests_run++;                                                            \
    if (condition) {                                                        \
        tests_passed++;                                                     \
    } else {                                                                \
        tests_failed++;                                                     \
        fprintf(stderr, "  FAIL [%s:%d] %s\n",                            \
                __FILE__, __LINE__, (msg));                                 \
    }                                                                       \
} while (0)

#define ASSERT_NEAR(expected, actual, tol, msg) do {                        \
    tests_run++;                                                            \
    float _e = (expected), _a = (actual);                                   \
    if (fabsf(_e - _a) <= (tol)) {                                         \
        tests_passed++;                                                     \
    } else {                                                                \
        tests_failed++;                                                     \
        fprintf(stderr, "  FAIL [%s:%d] %s: expected %.6f, got %.6f\n",    \
                __FILE__, __LINE__, (msg), (double)_e, (double)_a);        \
    }                                                                       \
} while (0)

#define ASSERT_VEC3_NEAR(expected, actual, tol, msg) do {                   \
    ASSERT_NEAR((expected).x, (actual).x, tol, msg " .x");                 \
    ASSERT_NEAR((expected).y, (actual).y, tol, msg " .y");                 \
    ASSERT_NEAR((expected).z, (actual).z, tol, msg " .z");                 \
} while (0)

#define RUN_TEST(fn) do {                                                   \
    printf("  %s ...\n", #fn);                                            \
    fn();                                                                   \
} while (0)

/* ================================================================== */
/* Vec3 Tests                                                           */
/* ================================================================== */

static void test_vec3_basics(void) {
    CmVec3 a = cm_vec3(1, 2, 3);
    CmVec3 b = cm_vec3(4, 5, 6);

    CmVec3 sum = cm_vec3_add(a, b);
    ASSERT_VEC3_NEAR(cm_vec3(5, 7, 9), sum, CM_EPSILON, "vec3 add");

    CmVec3 diff = cm_vec3_sub(a, b);
    ASSERT_VEC3_NEAR(cm_vec3(-3, -3, -3), diff, CM_EPSILON, "vec3 sub");

    CmVec3 scaled = cm_vec3_scale(a, 2.0f);
    ASSERT_VEC3_NEAR(cm_vec3(2, 4, 6), scaled, CM_EPSILON, "vec3 scale");

    CmVec3 neg = cm_vec3_neg(a);
    ASSERT_VEC3_NEAR(cm_vec3(-1, -2, -3), neg, CM_EPSILON, "vec3 neg");
}

static void test_vec3_dot_cross(void) {
    CmVec3 a = cm_vec3(1, 0, 0);
    CmVec3 b = cm_vec3(0, 1, 0);

    ASSERT_NEAR(0.0f, cm_vec3_dot(a, b), CM_EPSILON, "perpendicular dot");
    ASSERT_NEAR(1.0f, cm_vec3_dot(a, a), CM_EPSILON, "parallel dot");

    CmVec3 c = cm_vec3_cross(a, b);
    ASSERT_VEC3_NEAR(cm_vec3(0, 0, 1), c, CM_EPSILON, "cross X×Y=Z");

    CmVec3 d = cm_vec3_cross(b, a);
    ASSERT_VEC3_NEAR(cm_vec3(0, 0, -1), d, CM_EPSILON, "cross Y×X=-Z");
}

static void test_vec3_length_normalize(void) {
    CmVec3 v = cm_vec3(3, 4, 0);
    ASSERT_NEAR(25.0f, cm_vec3_length_sq(v), CM_EPSILON, "length squared");
    ASSERT_NEAR(5.0f, cm_vec3_length(v), CM_EPSILON, "length");

    CmVec3 n = cm_vec3_normalize(v);
    ASSERT_NEAR(1.0f, cm_vec3_length(n), 1e-5f, "normalized length");
    ASSERT_NEAR(0.6f, n.x, 1e-5f, "normalized x");
    ASSERT_NEAR(0.8f, n.y, 1e-5f, "normalized y");

    /* Zero vector normalization should not crash */
    CmVec3 z = cm_vec3_normalize(cm_vec3_zero());
    ASSERT_NEAR(0.0f, cm_vec3_length(z), CM_EPSILON, "normalize zero");
}

static void test_vec3_distance_lerp(void) {
    CmVec3 a = cm_vec3(0, 0, 0);
    CmVec3 b = cm_vec3(3, 4, 0);
    ASSERT_NEAR(5.0f, cm_vec3_distance(a, b), CM_EPSILON, "distance");

    CmVec3 mid = cm_vec3_lerp(a, b, 0.5f);
    ASSERT_VEC3_NEAR(cm_vec3(1.5f, 2.0f, 0), mid, CM_EPSILON, "lerp midpoint");

    CmVec3 start = cm_vec3_lerp(a, b, 0.0f);
    ASSERT_VEC3_NEAR(a, start, CM_EPSILON, "lerp t=0");

    CmVec3 end = cm_vec3_lerp(a, b, 1.0f);
    ASSERT_VEC3_NEAR(b, end, CM_EPSILON, "lerp t=1");
}

static void test_vec3_reflect(void) {
    CmVec3 v = cm_vec3(1, -1, 0);
    CmVec3 n = cm_vec3(0, 1, 0);
    CmVec3 r = cm_vec3_reflect(v, n);
    ASSERT_VEC3_NEAR(cm_vec3(1, 1, 0), r, CM_EPSILON, "reflect off Y plane");
}

static void test_vec3_min_max(void) {
    CmVec3 a = cm_vec3(1, 5, 3);
    CmVec3 b = cm_vec3(4, 2, 6);
    ASSERT_VEC3_NEAR(cm_vec3(1, 2, 3), cm_vec3_min(a, b), CM_EPSILON, "min");
    ASSERT_VEC3_NEAR(cm_vec3(4, 5, 6), cm_vec3_max(a, b), CM_EPSILON, "max");
}

/* ================================================================== */
/* Vec4 Tests                                                           */
/* ================================================================== */

static void test_vec4_basics(void) {
    CmVec4 v = cm_vec4(1, 2, 3, 4);
    ASSERT_NEAR(1.0f, v.x, CM_EPSILON, "vec4 x");
    ASSERT_NEAR(4.0f, v.w, CM_EPSILON, "vec4 w");

    CmVec3 xyz = cm_vec4_xyz(v);
    ASSERT_VEC3_NEAR(cm_vec3(1, 2, 3), xyz, CM_EPSILON, "vec4 xyz");

    CmVec4 from3 = cm_vec4_from_vec3(cm_vec3(5, 6, 7), 1.0f);
    ASSERT_NEAR(5.0f, from3.x, CM_EPSILON, "from_vec3 x");
    ASSERT_NEAR(1.0f, from3.w, CM_EPSILON, "from_vec3 w");

    CmVec4 a = cm_vec4(1, 0, 0, 0);
    CmVec4 b = cm_vec4(0, 1, 0, 0);
    ASSERT_NEAR(0.0f, cm_vec4_dot(a, b), CM_EPSILON, "vec4 dot perp");
    ASSERT_NEAR(1.0f, cm_vec4_dot(a, a), CM_EPSILON, "vec4 dot self");
}

/* ================================================================== */
/* Quaternion Tests                                                     */
/* ================================================================== */

static void test_quat_identity(void) {
    CmQuat q = cm_quat_identity();
    ASSERT_NEAR(1.0f, q.w, CM_EPSILON, "identity w");
    ASSERT_NEAR(0.0f, q.x, CM_EPSILON, "identity x");
    ASSERT_NEAR(1.0f, cm_quat_length(q), CM_EPSILON, "identity length");
}

static void test_quat_axis_angle(void) {
    /* 90° around Y axis */
    CmQuat q = cm_quat_from_axis_angle(cm_vec3(0, 1, 0), CM_PI * 0.5f);
    ASSERT_NEAR(1.0f, cm_quat_length(q), 1e-5f, "from_axis_angle length");

    /* Rotate X-axis → should become Z-axis (approximately) */
    CmVec3 rotated = cm_quat_rotate_vec3(q, cm_vec3(1, 0, 0));
    ASSERT_NEAR(0.0f, rotated.x, 1e-5f, "rotate 90Y: x");
    ASSERT_NEAR(0.0f, rotated.y, 1e-5f, "rotate 90Y: y");
    ASSERT_NEAR(-1.0f, rotated.z, 1e-5f, "rotate 90Y: z");
}

static void test_quat_mul_inverse(void) {
    CmQuat a = cm_quat_from_axis_angle(cm_vec3(0, 1, 0), CM_PI * 0.25f);
    CmQuat b = cm_quat_inverse(a);
    CmQuat product = cm_quat_mul(a, b);

    /* Should be ~identity */
    ASSERT_NEAR(1.0f, product.w, 1e-5f, "q * q^-1 w");
    ASSERT_NEAR(0.0f, product.x, 1e-5f, "q * q^-1 x");
    ASSERT_NEAR(0.0f, product.y, 1e-5f, "q * q^-1 y");
    ASSERT_NEAR(0.0f, product.z, 1e-5f, "q * q^-1 z");
}

static void test_quat_slerp(void) {
    CmQuat a = cm_quat_identity();
    CmQuat b = cm_quat_from_axis_angle(cm_vec3(0, 1, 0), CM_PI * 0.5f);

    CmQuat mid = cm_quat_slerp(a, b, 0.5f);
    ASSERT_NEAR(1.0f, cm_quat_length(mid), 1e-5f, "slerp midpoint length");

    /* Midpoint should be ~22.5° rotation */
    CmVec3 v = cm_quat_rotate_vec3(mid, cm_vec3(1, 0, 0));
    float expected_angle = CM_PI * 0.25f;
    ASSERT_NEAR(cosf(expected_angle), v.x, 1e-4f, "slerp midpoint rotation x");

    /* t=0 should be identity */
    CmQuat start = cm_quat_slerp(a, b, 0.0f);
    CmVec3 sv = cm_quat_rotate_vec3(start, cm_vec3(1, 0, 0));
    ASSERT_NEAR(1.0f, sv.x, 1e-5f, "slerp t=0 x");

    /* t=1 should match b */
    CmQuat end = cm_quat_slerp(a, b, 1.0f);
    CmVec3 ev = cm_quat_rotate_vec3(end, cm_vec3(1, 0, 0));
    CmVec3 bv = cm_quat_rotate_vec3(b, cm_vec3(1, 0, 0));
    ASSERT_NEAR(bv.x, ev.x, 1e-5f, "slerp t=1 x");
    ASSERT_NEAR(bv.z, ev.z, 1e-5f, "slerp t=1 z");
}

static void test_quat_conjugate(void) {
    CmQuat q = cm_quat(1, 2, 3, 4);
    CmQuat c = cm_quat_conjugate(q);
    ASSERT_NEAR(-1.0f, c.x, CM_EPSILON, "conjugate x");
    ASSERT_NEAR(-2.0f, c.y, CM_EPSILON, "conjugate y");
    ASSERT_NEAR(-3.0f, c.z, CM_EPSILON, "conjugate z");
    ASSERT_NEAR(4.0f, c.w, CM_EPSILON, "conjugate w");
}

/* ================================================================== */
/* Mat4 Tests                                                           */
/* ================================================================== */

static void test_mat4_identity(void) {
    CmMat4 m = cm_mat4_identity();
    CmVec3 p = cm_vec3(1, 2, 3);
    CmVec3 r = cm_mat4_mul_point(m, p);
    ASSERT_VEC3_NEAR(p, r, CM_EPSILON, "identity * point");
}

static void test_mat4_translate(void) {
    CmMat4 t = cm_mat4_translate(cm_vec3(10, 20, 30));
    CmVec3 p = cm_mat4_mul_point(t, cm_vec3(1, 2, 3));
    ASSERT_VEC3_NEAR(cm_vec3(11, 22, 33), p, CM_EPSILON, "translate point");

    /* Direction should NOT be affected by translation */
    CmVec3 d = cm_mat4_mul_dir(t, cm_vec3(1, 0, 0));
    ASSERT_VEC3_NEAR(cm_vec3(1, 0, 0), d, CM_EPSILON, "translate dir unchanged");
}

static void test_mat4_scale(void) {
    CmMat4 s = cm_mat4_scale(cm_vec3(2, 3, 4));
    CmVec3 p = cm_mat4_mul_point(s, cm_vec3(1, 1, 1));
    ASSERT_VEC3_NEAR(cm_vec3(2, 3, 4), p, CM_EPSILON, "scale point");
}

static void test_mat4_rotation(void) {
    /* 90° around Z should send X → Y */
    CmMat4 rz = cm_mat4_rotate_z(CM_PI * 0.5f);
    CmVec3 p = cm_mat4_mul_point(rz, cm_vec3(1, 0, 0));
    ASSERT_NEAR(0.0f, p.x, 1e-5f, "rotZ(90) x");
    ASSERT_NEAR(1.0f, p.y, 1e-5f, "rotZ(90) y");
    ASSERT_NEAR(0.0f, p.z, 1e-5f, "rotZ(90) z");
}

static void test_mat4_mul(void) {
    CmMat4 a = cm_mat4_translate(cm_vec3(1, 0, 0));
    CmMat4 b = cm_mat4_scale(cm_vec3(2, 2, 2));
    /* Scale first, then translate: T * S */
    CmMat4 ts = cm_mat4_mul(a, b);
    CmVec3 p = cm_mat4_mul_point(ts, cm_vec3(1, 0, 0));
    ASSERT_VEC3_NEAR(cm_vec3(3, 0, 0), p, CM_EPSILON, "T*S*(1,0,0)");
}

static void test_mat4_from_quat(void) {
    CmQuat q = cm_quat_from_axis_angle(cm_vec3(0, 0, 1), CM_PI * 0.5f);
    CmMat4 m = cm_mat4_from_quat(q);
    CmVec3 p = cm_mat4_mul_point(m, cm_vec3(1, 0, 0));
    ASSERT_NEAR(0.0f, p.x, 1e-5f, "quat→mat rotZ x");
    ASSERT_NEAR(1.0f, p.y, 1e-5f, "quat→mat rotZ y");
}

static void test_mat4_trs(void) {
    CmQuat rot = cm_quat_identity();
    CmMat4 m = cm_mat4_trs(cm_vec3(10, 0, 0), rot, cm_vec3(2, 2, 2));
    CmVec3 p = cm_mat4_mul_point(m, cm_vec3(1, 0, 0));
    ASSERT_VEC3_NEAR(cm_vec3(12, 0, 0), p, 1e-5f, "TRS point");
}

static void test_mat4_transpose(void) {
    CmMat4 m = cm_mat4_identity();
    m.m[4] = 5.0f;   /* row 0, col 1 */
    CmMat4 t = cm_mat4_transpose(m);
    ASSERT_NEAR(5.0f, t.m[1], CM_EPSILON, "transpose [1][0]");
    ASSERT_NEAR(0.0f, t.m[4], CM_EPSILON, "transpose [0][1]");
}

/* ================================================================== */
/* Projection & View                                                    */
/* ================================================================== */

static void test_perspective(void) {
    CmMat4 p = cm_perspective(60.0f, 4.0f / 3.0f, 0.1f, 1000.0f);
    /* Should not be identity */
    ASSERT_TRUE(p.m[0] != 0.0f, "perspective [0][0] nonzero");
    ASSERT_TRUE(p.m[5] != 0.0f, "perspective [1][1] nonzero");
    ASSERT_TRUE(p.m[10] != 0.0f, "perspective [2][2] nonzero");
    /* Point at origin looking down -Z: near plane point */
    CmVec4 near_pt = cm_mat4_mul_vec4(p, cm_vec4(0, 0, -0.1f, 1));
    ASSERT_TRUE(near_pt.w != 0.0f, "perspective w != 0 at near");
}

static void test_lookat(void) {
    CmMat4 v = cm_lookat(cm_vec3(0, 0, 5), cm_vec3(0, 0, 0), cm_vec3(0, 1, 0));
    /* Origin in world space should map to (0, 0, -5) in view space */
    CmVec3 p = cm_mat4_mul_point(v, cm_vec3(0, 0, 0));
    ASSERT_NEAR(0.0f, p.x, 1e-5f, "lookat origin x");
    ASSERT_NEAR(0.0f, p.y, 1e-5f, "lookat origin y");
    ASSERT_NEAR(-5.0f, p.z, 1e-4f, "lookat origin z");
}

static void test_ortho(void) {
    CmMat4 o = cm_ortho(-1, 1, -1, 1, -1, 1);
    /* Center should map to center */
    CmVec3 p = cm_mat4_mul_point(o, cm_vec3(0, 0, 0));
    ASSERT_NEAR(0.0f, p.x, 1e-5f, "ortho center x");
    ASSERT_NEAR(0.0f, p.y, 1e-5f, "ortho center y");
}

/* ================================================================== */
/* AABB Tests                                                           */
/* ================================================================== */

static void test_aabb_helpers(void) {
    CmAABB b = cm_aabb_from_center_half(cm_vec3(0, 0, 0), cm_vec3(1, 1, 1));
    ASSERT_VEC3_NEAR(cm_vec3(-1, -1, -1), b.min, CM_EPSILON, "aabb min");
    ASSERT_VEC3_NEAR(cm_vec3(1, 1, 1), b.max, CM_EPSILON, "aabb max");

    CmVec3 c = cm_aabb_center(b);
    ASSERT_VEC3_NEAR(cm_vec3(0, 0, 0), c, CM_EPSILON, "aabb center");

    ASSERT_TRUE(cm_aabb_contains_point(b, cm_vec3(0, 0, 0)), "contains origin");
    ASSERT_TRUE(!cm_aabb_contains_point(b, cm_vec3(2, 0, 0)), "!contains outside");

    CmAABB merged = cm_aabb_merge(b, (CmAABB){ cm_vec3(0, 0, 0), cm_vec3(5, 5, 5) });
    ASSERT_VEC3_NEAR(cm_vec3(-1, -1, -1), merged.min, CM_EPSILON, "merge min");
    ASSERT_VEC3_NEAR(cm_vec3(5, 5, 5), merged.max, CM_EPSILON, "merge max");
}

/* ================================================================== */
/* Intersection Tests                                                   */
/* ================================================================== */

static void test_sphere_vs_sphere(void) {
    CmSphere a = { cm_vec3(0, 0, 0), 1.0f };
    CmSphere b = { cm_vec3(1.5f, 0, 0), 1.0f };
    CmContact c = {0};

    ASSERT_TRUE(cm_sphere_vs_sphere(a, b, &c), "overlapping spheres");
    ASSERT_NEAR(0.5f, c.depth, 1e-5f, "sphere-sphere depth");
    ASSERT_NEAR(1.0f, c.normal.x, 1e-5f, "sphere-sphere normal x");

    CmSphere far = { cm_vec3(10, 0, 0), 1.0f };
    ASSERT_TRUE(!cm_sphere_vs_sphere(a, far, NULL), "non-overlapping spheres");
}

static void test_sphere_vs_aabb(void) {
    CmSphere s = { cm_vec3(0, 1.5f, 0), 1.0f };
    CmAABB b = { cm_vec3(-1, -1, -1), cm_vec3(1, 1, 1) };
    CmContact c = {0};

    ASSERT_TRUE(cm_sphere_vs_aabb(s, b, &c), "sphere-aabb overlap");
    ASSERT_NEAR(0.5f, c.depth, 1e-5f, "sphere-aabb depth");

    CmSphere far = { cm_vec3(0, 5, 0), 1.0f };
    ASSERT_TRUE(!cm_sphere_vs_aabb(far, b, NULL), "sphere-aabb no overlap");
}

static void test_sphere_vs_plane(void) {
    CmSphere s = { cm_vec3(0, 0.5f, 0), 1.0f };
    CmPlane p = { cm_vec3(0, 1, 0), 0.0f };  /* Y=0 plane */
    CmContact c = {0};

    ASSERT_TRUE(cm_sphere_vs_plane(s, p, &c), "sphere-plane overlap");
    ASSERT_NEAR(0.5f, c.depth, 1e-5f, "sphere-plane depth");

    CmSphere high = { cm_vec3(0, 5, 0), 1.0f };
    ASSERT_TRUE(!cm_sphere_vs_plane(high, p, NULL), "sphere-plane no overlap");
}

static void test_aabb_vs_aabb(void) {
    CmAABB a = { cm_vec3(0, 0, 0), cm_vec3(2, 2, 2) };
    CmAABB b = { cm_vec3(1, 1, 1), cm_vec3(3, 3, 3) };
    CmContact c = {0};

    ASSERT_TRUE(cm_aabb_vs_aabb(a, b, &c), "aabb-aabb overlap");
    ASSERT_TRUE(c.depth > 0.0f, "aabb-aabb positive depth");

    CmAABB far = { cm_vec3(10, 10, 10), cm_vec3(12, 12, 12) };
    ASSERT_TRUE(!cm_aabb_vs_aabb(a, far, NULL), "aabb-aabb no overlap");
}

static void test_capsule_vs_capsule(void) {
    CmCapsule a = { cm_vec3(0, 0, 0), cm_vec3(0, 2, 0), 0.5f };
    CmCapsule b = { cm_vec3(0.8f, 0, 0), cm_vec3(0.8f, 2, 0), 0.5f };
    CmContact c = {0};

    ASSERT_TRUE(cm_capsule_vs_capsule(a, b, &c), "capsule-capsule overlap");
    ASSERT_TRUE(c.depth > 0.0f, "capsule-capsule positive depth");

    CmCapsule far = { cm_vec3(10, 0, 0), cm_vec3(10, 2, 0), 0.5f };
    ASSERT_TRUE(!cm_capsule_vs_capsule(a, far, NULL), "capsule-capsule no overlap");
}

/* ================================================================== */
/* Ray Cast Tests                                                       */
/* ================================================================== */

static void test_ray_vs_sphere(void) {
    CmRay ray = { cm_vec3(0, 0, -5), cm_vec3(0, 0, 1) };
    CmSphere s = { cm_vec3(0, 0, 0), 1.0f };
    float t = 0;

    ASSERT_TRUE(cm_ray_vs_sphere(ray, s, &t), "ray hits sphere");
    ASSERT_NEAR(4.0f, t, 1e-5f, "ray-sphere t");

    CmRay miss = { cm_vec3(0, 5, -5), cm_vec3(0, 0, 1) };
    ASSERT_TRUE(!cm_ray_vs_sphere(miss, s, &t), "ray misses sphere");
}

static void test_ray_vs_plane(void) {
    CmRay ray = { cm_vec3(0, 5, 0), cm_vec3(0, -1, 0) };
    CmPlane p = { cm_vec3(0, 1, 0), 0.0f };
    float t = 0;

    ASSERT_TRUE(cm_ray_vs_plane(ray, p, &t), "ray hits plane");
    ASSERT_NEAR(5.0f, t, 1e-5f, "ray-plane t");

    /* Parallel ray should miss */
    CmRay parallel = { cm_vec3(0, 5, 0), cm_vec3(1, 0, 0) };
    ASSERT_TRUE(!cm_ray_vs_plane(parallel, p, &t), "parallel ray misses plane");
}

static void test_ray_vs_aabb(void) {
    CmRay ray = { cm_vec3(0, 0, -5), cm_vec3(0, 0, 1) };
    CmAABB b = { cm_vec3(-1, -1, -1), cm_vec3(1, 1, 1) };
    float t = 0;

    ASSERT_TRUE(cm_ray_vs_aabb(ray, b, &t), "ray hits aabb");
    ASSERT_NEAR(4.0f, t, 1e-5f, "ray-aabb t");

    CmRay miss = { cm_vec3(0, 5, -5), cm_vec3(0, 0, 1) };
    ASSERT_TRUE(!cm_ray_vs_aabb(miss, b, &t), "ray misses aabb");
}

static void test_ray_vs_triangle(void) {
    CmRay ray = { cm_vec3(0.25f, 0.25f, -1), cm_vec3(0, 0, 1) };
    CmVec3 v0 = cm_vec3(0, 0, 0);
    CmVec3 v1 = cm_vec3(1, 0, 0);
    CmVec3 v2 = cm_vec3(0, 1, 0);
    float t = 0;

    ASSERT_TRUE(cm_ray_vs_triangle(ray, v0, v1, v2, &t), "ray hits triangle");
    ASSERT_NEAR(1.0f, t, 1e-5f, "ray-triangle t");

    CmRay miss = { cm_vec3(5, 5, -1), cm_vec3(0, 0, 1) };
    ASSERT_TRUE(!cm_ray_vs_triangle(miss, v0, v1, v2, &t), "ray misses triangle");
}

/* ================================================================== */
/* Frustum Tests                                                        */
/* ================================================================== */

static void test_frustum_culling(void) {
    CmMat4 proj = cm_perspective(60.0f, 4.0f / 3.0f, 0.1f, 100.0f);
    CmMat4 view = cm_lookat(cm_vec3(0, 0, 10), cm_vec3(0, 0, 0), cm_vec3(0, 1, 0));
    CmMat4 vp = cm_mat4_mul(proj, view);
    CmFrustum f = cm_frustum_from_vp(vp);

    /* Sphere at origin should be inside (camera at z=10 looking at origin) */
    CmSphere inside = { cm_vec3(0, 0, 0), 1.0f };
    ASSERT_TRUE(cm_frustum_contains_sphere(&f, inside), "sphere inside frustum");

    /* Sphere way behind camera should be outside */
    CmSphere behind = { cm_vec3(0, 0, 50), 1.0f };
    ASSERT_TRUE(!cm_frustum_contains_sphere(&f, behind), "sphere behind frustum");

    /* AABB at origin */
    CmAABB inside_box = { cm_vec3(-1, -1, -1), cm_vec3(1, 1, 1) };
    ASSERT_TRUE(cm_frustum_contains_aabb(&f, inside_box), "aabb inside frustum");

    /* AABB way off to the side */
    CmAABB outside_box = { cm_vec3(200, 200, 200), cm_vec3(201, 201, 201) };
    ASSERT_TRUE(!cm_frustum_contains_aabb(&f, outside_box), "aabb outside frustum");
}

/* ================================================================== */
/* Utility Tests                                                        */
/* ================================================================== */

static void test_utility(void) {
    ASSERT_NEAR(0.5f, cm_clampf(0.5f, 0.0f, 1.0f), CM_EPSILON, "clamp middle");
    ASSERT_NEAR(0.0f, cm_clampf(-1.0f, 0.0f, 1.0f), CM_EPSILON, "clamp low");
    ASSERT_NEAR(1.0f, cm_clampf(2.0f, 0.0f, 1.0f), CM_EPSILON, "clamp high");

    ASSERT_NEAR(0.5f, cm_lerpf(0.0f, 1.0f, 0.5f), CM_EPSILON, "lerpf 0.5");
    ASSERT_NEAR(2.0f, cm_minf(2.0f, 5.0f), CM_EPSILON, "minf");
    ASSERT_NEAR(5.0f, cm_maxf(2.0f, 5.0f), CM_EPSILON, "maxf");
    ASSERT_NEAR(3.0f, cm_absf(-3.0f), CM_EPSILON, "absf");
}

/* ================================================================== */
/* Main                                                                 */
/* ================================================================== */

int main(void) {
    printf("test_math\n");

    printf(" Vec3:\n");
    RUN_TEST(test_vec3_basics);
    RUN_TEST(test_vec3_dot_cross);
    RUN_TEST(test_vec3_length_normalize);
    RUN_TEST(test_vec3_distance_lerp);
    RUN_TEST(test_vec3_reflect);
    RUN_TEST(test_vec3_min_max);

    printf(" Vec4:\n");
    RUN_TEST(test_vec4_basics);

    printf(" Quaternion:\n");
    RUN_TEST(test_quat_identity);
    RUN_TEST(test_quat_axis_angle);
    RUN_TEST(test_quat_mul_inverse);
    RUN_TEST(test_quat_slerp);
    RUN_TEST(test_quat_conjugate);

    printf(" Mat4:\n");
    RUN_TEST(test_mat4_identity);
    RUN_TEST(test_mat4_translate);
    RUN_TEST(test_mat4_scale);
    RUN_TEST(test_mat4_rotation);
    RUN_TEST(test_mat4_mul);
    RUN_TEST(test_mat4_from_quat);
    RUN_TEST(test_mat4_trs);
    RUN_TEST(test_mat4_transpose);

    printf(" Projection & View:\n");
    RUN_TEST(test_perspective);
    RUN_TEST(test_lookat);
    RUN_TEST(test_ortho);

    printf(" AABB:\n");
    RUN_TEST(test_aabb_helpers);

    printf(" Intersection:\n");
    RUN_TEST(test_sphere_vs_sphere);
    RUN_TEST(test_sphere_vs_aabb);
    RUN_TEST(test_sphere_vs_plane);
    RUN_TEST(test_aabb_vs_aabb);
    RUN_TEST(test_capsule_vs_capsule);

    printf(" Ray Casts:\n");
    RUN_TEST(test_ray_vs_sphere);
    RUN_TEST(test_ray_vs_plane);
    RUN_TEST(test_ray_vs_aabb);
    RUN_TEST(test_ray_vs_triangle);

    printf(" Frustum:\n");
    RUN_TEST(test_frustum_culling);

    printf(" Utility:\n");
    RUN_TEST(test_utility);

    printf("\n%d tests: %d passed, %d failed\n",
           tests_run, tests_passed, tests_failed);
    return tests_failed > 0 ? 1 : 0;
}
