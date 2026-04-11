/*
 * calynda_math.c — Implementation of projection, view matrices and
 * collision intersection tests for Calynda Wii.
 *
 * On Wii builds, perspective/lookat wrap libogc's guPerspective/guLookAt
 * and convert from Mtx44/Mtx to CmMat4. On host builds, standalone
 * implementations are provided.
 */

#include "calynda_math.h"

#include <math.h>

/* ================================================================== */
/* Projection & View                                                    */
/* ================================================================== */

#ifdef CALYNDA_WII_BUILD

/*
 * libogc uses row-major Mtx44 (4x4) and Mtx (3x4).
 * We convert to our column-major CmMat4.
 */

static CmMat4 cm_mtx44_to_mat4(Mtx44 src) {
    CmMat4 r;
    for (int row = 0; row < 4; row++)
        for (int col = 0; col < 4; col++)
            r.m[col * 4 + row] = src[row][col];
    return r;
}

static CmMat4 cm_mtx_to_mat4(Mtx src) {
    CmMat4 r = cm_mat4_identity();
    for (int row = 0; row < 3; row++)
        for (int col = 0; col < 4; col++)
            r.m[col * 4 + row] = src[row][col];
    /* row 3 stays 0 0 0 1 from identity */
    return r;
}

CmMat4 cm_perspective(float fov_y_deg, float aspect, float near_z, float far_z) {
    Mtx44 proj;
    guPerspective(proj, fov_y_deg, aspect, near_z, far_z);
    return cm_mtx44_to_mat4(proj);
}

CmMat4 cm_lookat(CmVec3 eye, CmVec3 target, CmVec3 up) {
    Mtx view;
    guVector ge = { eye.x, eye.y, eye.z };
    guVector gt = { target.x, target.y, target.z };
    guVector gu = { up.x, up.y, up.z };
    guLookAt(view, &ge, &gu, &gt);
    return cm_mtx_to_mat4(view);
}

CmMat4 cm_ortho(float left, float right, float bottom, float top,
                 float near_z, float far_z) {
    Mtx44 proj;
    guOrtho(proj, top, bottom, left, right, near_z, far_z);
    return cm_mtx44_to_mat4(proj);
}

#else /* Host / standalone */

CmMat4 cm_perspective(float fov_y_deg, float aspect, float near_z, float far_z) {
    float fov_rad = fov_y_deg * CM_DEG2RAD;
    float tan_half = tanf(fov_rad * 0.5f);
    CmMat4 r = cm_mat4_zero();
    r.m[0]  = 1.0f / (aspect * tan_half);
    r.m[5]  = 1.0f / tan_half;
    r.m[10] = -(far_z + near_z) / (far_z - near_z);
    r.m[11] = -1.0f;
    r.m[14] = -(2.0f * far_z * near_z) / (far_z - near_z);
    return r;
}

CmMat4 cm_lookat(CmVec3 eye, CmVec3 target, CmVec3 up) {
    CmVec3 f = cm_vec3_normalize(cm_vec3_sub(target, eye));
    CmVec3 s = cm_vec3_normalize(cm_vec3_cross(f, up));
    CmVec3 u = cm_vec3_cross(s, f);

    CmMat4 r = cm_mat4_identity();
    r.m[0] =  s.x;  r.m[4] =  s.y;  r.m[8]  =  s.z;
    r.m[1] =  u.x;  r.m[5] =  u.y;  r.m[9]  =  u.z;
    r.m[2] = -f.x;  r.m[6] = -f.y;  r.m[10] = -f.z;
    r.m[12] = -cm_vec3_dot(s, eye);
    r.m[13] = -cm_vec3_dot(u, eye);
    r.m[14] =  cm_vec3_dot(f, eye);
    return r;
}

CmMat4 cm_ortho(float left, float right, float bottom, float top,
                 float near_z, float far_z) {
    CmMat4 r = cm_mat4_zero();
    r.m[0]  =  2.0f / (right - left);
    r.m[5]  =  2.0f / (top - bottom);
    r.m[10] = -2.0f / (far_z - near_z);
    r.m[12] = -(right + left) / (right - left);
    r.m[13] = -(top + bottom) / (top - bottom);
    r.m[14] = -(far_z + near_z) / (far_z - near_z);
    r.m[15] = 1.0f;
    return r;
}

#endif /* CALYNDA_WII_BUILD */

/* ================================================================== */
/* Intersection Tests                                                   */
/* ================================================================== */

bool cm_sphere_vs_sphere(CmSphere a, CmSphere b, CmContact *out) {
    CmVec3 diff = cm_vec3_sub(b.center, a.center);
    float dist_sq = cm_vec3_length_sq(diff);
    float r_sum = a.radius + b.radius;
    if (dist_sq > r_sum * r_sum) return false;

    float dist = sqrtf(dist_sq);
    if (out) {
        if (dist < CM_EPSILON) {
            out->normal = cm_vec3_up();
            out->depth = r_sum;
            out->point = a.center;
        } else {
            out->normal = cm_vec3_scale(diff, 1.0f / dist);
            out->depth = r_sum - dist;
            out->point = cm_vec3_add(a.center, cm_vec3_scale(out->normal, a.radius - out->depth * 0.5f));
        }
        out->hit = true;
    }
    return true;
}

bool cm_sphere_vs_aabb(CmSphere s, CmAABB b, CmContact *out) {
    /* Find closest point on AABB to sphere center */
    CmVec3 closest;
    closest.x = cm_clampf(s.center.x, b.min.x, b.max.x);
    closest.y = cm_clampf(s.center.y, b.min.y, b.max.y);
    closest.z = cm_clampf(s.center.z, b.min.z, b.max.z);

    CmVec3 diff = cm_vec3_sub(s.center, closest);
    float dist_sq = cm_vec3_length_sq(diff);
    if (dist_sq > s.radius * s.radius) return false;

    if (out) {
        float dist = sqrtf(dist_sq);
        if (dist < CM_EPSILON) {
            /* Sphere center inside AABB — push out along shortest axis */
            CmVec3 center = cm_aabb_center(b);
            CmVec3 half = cm_aabb_half_extents(b);
            CmVec3 d = cm_vec3_sub(s.center, center);
            float ax = half.x - cm_absf(d.x);
            float ay = half.y - cm_absf(d.y);
            float az = half.z - cm_absf(d.z);
            if (ax <= ay && ax <= az) {
                out->normal = cm_vec3(d.x >= 0 ? 1.0f : -1.0f, 0, 0);
                out->depth = ax + s.radius;
            } else if (ay <= az) {
                out->normal = cm_vec3(0, d.y >= 0 ? 1.0f : -1.0f, 0);
                out->depth = ay + s.radius;
            } else {
                out->normal = cm_vec3(0, 0, d.z >= 0 ? 1.0f : -1.0f);
                out->depth = az + s.radius;
            }
            out->point = closest;
        } else {
            out->normal = cm_vec3_scale(diff, 1.0f / dist);
            out->depth = s.radius - dist;
            out->point = closest;
        }
        out->hit = true;
    }
    return true;
}

bool cm_sphere_vs_plane(CmSphere s, CmPlane p, CmContact *out) {
    float d = cm_vec3_dot(p.normal, s.center) - p.distance;
    if (d > s.radius) return false;

    if (out) {
        out->normal = p.normal;
        out->depth = s.radius - d;
        out->point = cm_vec3_sub(s.center, cm_vec3_scale(p.normal, d));
        out->hit = true;
    }
    return true;
}

bool cm_aabb_vs_aabb(CmAABB a, CmAABB b, CmContact *out) {
    if (a.max.x < b.min.x || a.min.x > b.max.x) return false;
    if (a.max.y < b.min.y || a.min.y > b.max.y) return false;
    if (a.max.z < b.min.z || a.min.z > b.max.z) return false;

    if (out) {
        /* Find minimum overlap axis */
        float ox = cm_minf(a.max.x, b.max.x) - cm_maxf(a.min.x, b.min.x);
        float oy = cm_minf(a.max.y, b.max.y) - cm_maxf(a.min.y, b.min.y);
        float oz = cm_minf(a.max.z, b.max.z) - cm_maxf(a.min.z, b.min.z);

        CmVec3 ca = cm_aabb_center(a);
        CmVec3 cb = cm_aabb_center(b);
        CmVec3 d = cm_vec3_sub(cb, ca);

        if (ox <= oy && ox <= oz) {
            out->normal = cm_vec3(d.x >= 0 ? 1.0f : -1.0f, 0, 0);
            out->depth = ox;
        } else if (oy <= oz) {
            out->normal = cm_vec3(0, d.y >= 0 ? 1.0f : -1.0f, 0);
            out->depth = oy;
        } else {
            out->normal = cm_vec3(0, 0, d.z >= 0 ? 1.0f : -1.0f);
            out->depth = oz;
        }
        out->point = cm_vec3_scale(cm_vec3_add(
            cm_vec3_max(a.min, b.min),
            cm_vec3_min(a.max, b.max)
        ), 0.5f);
        out->hit = true;
    }
    return true;
}

/* Closest point on line segment to a point */
static CmVec3 cm_closest_point_on_segment(CmVec3 a, CmVec3 b, CmVec3 p) {
    CmVec3 ab = cm_vec3_sub(b, a);
    float t = cm_vec3_dot(cm_vec3_sub(p, a), ab);
    float denom = cm_vec3_dot(ab, ab);
    if (denom < CM_EPSILON) return a;
    t = cm_clampf(t / denom, 0.0f, 1.0f);
    return cm_vec3_add(a, cm_vec3_scale(ab, t));
}

/* Closest points between two line segments */
static void cm_closest_points_segments(CmVec3 a0, CmVec3 a1,
                                        CmVec3 b0, CmVec3 b1,
                                        CmVec3 *out_a, CmVec3 *out_b) {
    CmVec3 d1 = cm_vec3_sub(a1, a0);
    CmVec3 d2 = cm_vec3_sub(b1, b0);
    CmVec3 r  = cm_vec3_sub(a0, b0);

    float a = cm_vec3_dot(d1, d1);
    float e = cm_vec3_dot(d2, d2);
    float f = cm_vec3_dot(d2, r);

    float s, t;

    if (a < CM_EPSILON && e < CM_EPSILON) {
        *out_a = a0;
        *out_b = b0;
        return;
    }
    if (a < CM_EPSILON) {
        s = 0.0f;
        t = cm_clampf(f / e, 0.0f, 1.0f);
    } else {
        float c = cm_vec3_dot(d1, r);
        if (e < CM_EPSILON) {
            t = 0.0f;
            s = cm_clampf(-c / a, 0.0f, 1.0f);
        } else {
            float b_val = cm_vec3_dot(d1, d2);
            float denom = a * e - b_val * b_val;
            if (denom > CM_EPSILON) {
                s = cm_clampf((b_val * f - c * e) / denom, 0.0f, 1.0f);
            } else {
                s = 0.0f;
            }
            t = (b_val * s + f) / e;
            if (t < 0.0f) {
                t = 0.0f;
                s = cm_clampf(-c / a, 0.0f, 1.0f);
            } else if (t > 1.0f) {
                t = 1.0f;
                s = cm_clampf((b_val - c) / a, 0.0f, 1.0f);
            }
        }
    }

    *out_a = cm_vec3_add(a0, cm_vec3_scale(d1, s));
    *out_b = cm_vec3_add(b0, cm_vec3_scale(d2, t));
}

bool cm_capsule_vs_capsule(CmCapsule a, CmCapsule b, CmContact *out) {
    CmVec3 pa, pb;
    cm_closest_points_segments(a.a, a.b, b.a, b.b, &pa, &pb);

    CmVec3 diff = cm_vec3_sub(pb, pa);
    float dist_sq = cm_vec3_length_sq(diff);
    float r_sum = a.radius + b.radius;
    if (dist_sq > r_sum * r_sum) return false;

    if (out) {
        float dist = sqrtf(dist_sq);
        if (dist < CM_EPSILON) {
            out->normal = cm_vec3_up();
        } else {
            out->normal = cm_vec3_scale(diff, 1.0f / dist);
        }
        out->depth = r_sum - dist;
        out->point = cm_vec3_add(pa, cm_vec3_scale(out->normal, a.radius));
        out->hit = true;
    }
    return true;
}

/* ================================================================== */
/* Ray Casts                                                            */
/* ================================================================== */

bool cm_ray_vs_sphere(CmRay ray, CmSphere s, float *t) {
    CmVec3 oc = cm_vec3_sub(ray.origin, s.center);
    float b = cm_vec3_dot(oc, ray.direction);
    float c = cm_vec3_dot(oc, oc) - s.radius * s.radius;
    float disc = b * b - c;
    if (disc < 0.0f) return false;

    float sqrt_disc = sqrtf(disc);
    float t0 = -b - sqrt_disc;
    float t1 = -b + sqrt_disc;
    if (t1 < 0.0f) return false;

    *t = (t0 >= 0.0f) ? t0 : t1;
    return true;
}

bool cm_ray_vs_plane(CmRay ray, CmPlane p, float *t) {
    float denom = cm_vec3_dot(p.normal, ray.direction);
    if (cm_absf(denom) < CM_EPSILON) return false;

    *t = (p.distance - cm_vec3_dot(p.normal, ray.origin)) / denom;
    return *t >= 0.0f;
}

bool cm_ray_vs_aabb(CmRay ray, CmAABB b, float *t) {
    float tmin = -1e30f, tmax = 1e30f;
    float origin[3] = { ray.origin.x, ray.origin.y, ray.origin.z };
    float dir[3]    = { ray.direction.x, ray.direction.y, ray.direction.z };
    float bmin[3]   = { b.min.x, b.min.y, b.min.z };
    float bmax[3]   = { b.max.x, b.max.y, b.max.z };

    for (int i = 0; i < 3; i++) {
        if (cm_absf(dir[i]) < CM_EPSILON) {
            if (origin[i] < bmin[i] || origin[i] > bmax[i]) return false;
        } else {
            float inv_d = 1.0f / dir[i];
            float t1 = (bmin[i] - origin[i]) * inv_d;
            float t2 = (bmax[i] - origin[i]) * inv_d;
            if (t1 > t2) { float tmp = t1; t1 = t2; t2 = tmp; }
            if (t1 > tmin) tmin = t1;
            if (t2 < tmax) tmax = t2;
            if (tmin > tmax) return false;
        }
    }
    if (tmax < 0.0f) return false;
    *t = (tmin >= 0.0f) ? tmin : tmax;
    return true;
}

bool cm_ray_vs_triangle(CmRay ray, CmVec3 v0, CmVec3 v1, CmVec3 v2, float *t) {
    /* Möller–Trumbore intersection */
    CmVec3 e1 = cm_vec3_sub(v1, v0);
    CmVec3 e2 = cm_vec3_sub(v2, v0);
    CmVec3 h = cm_vec3_cross(ray.direction, e2);
    float a = cm_vec3_dot(e1, h);
    if (cm_absf(a) < CM_EPSILON) return false;

    float f = 1.0f / a;
    CmVec3 s = cm_vec3_sub(ray.origin, v0);
    float u = f * cm_vec3_dot(s, h);
    if (u < 0.0f || u > 1.0f) return false;

    CmVec3 q = cm_vec3_cross(s, e1);
    float v = f * cm_vec3_dot(ray.direction, q);
    if (v < 0.0f || u + v > 1.0f) return false;

    float tt = f * cm_vec3_dot(e2, q);
    if (tt < CM_EPSILON) return false;

    *t = tt;
    return true;
}

/* ================================================================== */
/* Frustum                                                              */
/* ================================================================== */

CmFrustum cm_frustum_from_vp(CmMat4 vp) {
    CmFrustum f;
    /* Extract planes from view-projection matrix (column-major) */
    /* Left:   row3 + row0 */
    f.planes[0].normal.x = vp.m[3]  + vp.m[0];
    f.planes[0].normal.y = vp.m[7]  + vp.m[4];
    f.planes[0].normal.z = vp.m[11] + vp.m[8];
    f.planes[0].distance = -(vp.m[15] + vp.m[12]);

    /* Right:  row3 - row0 */
    f.planes[1].normal.x = vp.m[3]  - vp.m[0];
    f.planes[1].normal.y = vp.m[7]  - vp.m[4];
    f.planes[1].normal.z = vp.m[11] - vp.m[8];
    f.planes[1].distance = -(vp.m[15] - vp.m[12]);

    /* Bottom: row3 + row1 */
    f.planes[2].normal.x = vp.m[3]  + vp.m[1];
    f.planes[2].normal.y = vp.m[7]  + vp.m[5];
    f.planes[2].normal.z = vp.m[11] + vp.m[9];
    f.planes[2].distance = -(vp.m[15] + vp.m[13]);

    /* Top:    row3 - row1 */
    f.planes[3].normal.x = vp.m[3]  - vp.m[1];
    f.planes[3].normal.y = vp.m[7]  - vp.m[5];
    f.planes[3].normal.z = vp.m[11] - vp.m[9];
    f.planes[3].distance = -(vp.m[15] - vp.m[13]);

    /* Near:   row3 + row2 */
    f.planes[4].normal.x = vp.m[3]  + vp.m[2];
    f.planes[4].normal.y = vp.m[7]  + vp.m[6];
    f.planes[4].normal.z = vp.m[11] + vp.m[10];
    f.planes[4].distance = -(vp.m[15] + vp.m[14]);

    /* Far:    row3 - row2 */
    f.planes[5].normal.x = vp.m[3]  - vp.m[2];
    f.planes[5].normal.y = vp.m[7]  - vp.m[6];
    f.planes[5].normal.z = vp.m[11] - vp.m[10];
    f.planes[5].distance = -(vp.m[15] - vp.m[14]);

    /* Normalize all planes */
    for (int i = 0; i < 6; i++) {
        float len = cm_vec3_length(f.planes[i].normal);
        if (len > CM_EPSILON) {
            float inv = 1.0f / len;
            f.planes[i].normal = cm_vec3_scale(f.planes[i].normal, inv);
            f.planes[i].distance *= inv;
        }
    }
    return f;
}

bool cm_frustum_contains_sphere(const CmFrustum *f, CmSphere s) {
    for (int i = 0; i < 6; i++) {
        float d = cm_vec3_dot(f->planes[i].normal, s.center) - f->planes[i].distance;
        if (d < -s.radius) return false;
    }
    return true;
}

bool cm_frustum_contains_aabb(const CmFrustum *f, CmAABB b) {
    for (int i = 0; i < 6; i++) {
        /* Test the positive vertex (furthest along plane normal) */
        CmVec3 pv;
        pv.x = (f->planes[i].normal.x >= 0) ? b.max.x : b.min.x;
        pv.y = (f->planes[i].normal.y >= 0) ? b.max.y : b.min.y;
        pv.z = (f->planes[i].normal.z >= 0) ? b.max.z : b.min.z;
        float d = cm_vec3_dot(f->planes[i].normal, pv) - f->planes[i].distance;
        if (d < 0.0f) return false;
    }
    return true;
}
