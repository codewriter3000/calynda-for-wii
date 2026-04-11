/*
 * calynda_physics.c — Physics engine implementation.
 */

#include "calynda_physics.h"
#include <math.h>
#include <string.h>
#include <stdio.h>

/* ================================================================== */
/* Internal helpers                                                     */
/* ================================================================== */

static inline float phys_clamp(float v, float lo, float hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

static inline float phys_min(float a, float b) { return a < b ? a : b; }
static inline float phys_max(float a, float b) { return a > b ? a : b; }

/* Compute inertia for common shapes */
static float compute_inertia(float mass, const PhysCollider *c) {
    if (mass <= 0) return 0;
    switch (c->type) {
        case PHYS_SHAPE_SPHERE:
            return (2.0f / 5.0f) * mass * c->sphere.radius * c->sphere.radius;
        case PHYS_SHAPE_AABB: {
            CmVec3 h = c->aabb.half_extents;
            float w2 = (2*h.x)*(2*h.x), ht2 = (2*h.y)*(2*h.y), d2 = (2*h.z)*(2*h.z);
            return (1.0f / 12.0f) * mass * (w2 + ht2 + d2);
        }
        case PHYS_SHAPE_CAPSULE: {
            float r = c->capsule.radius;
            float hh = c->capsule.half_height;
            /* approximate as cylinder + two hemispheres */
            float cyl_h = 2 * hh;
            return (1.0f / 12.0f) * mass * (3*r*r + cyl_h*cyl_h);
        }
        case PHYS_SHAPE_PLANE:
            return 0; /* infinite planes are always static */
    }
    return 0;
}

/* ================================================================== */
/* World                                                                */
/* ================================================================== */

void phys_world_init(PhysWorld *world) {
    memset(world, 0, sizeof(*world));
    world->gravity = PHYS_GRAVITY;
    world->solver_iterations = 4;
    world->water_enabled = false;
    world->water_y = 0;
    world->water_density = 1.0f;

    /* Init spatial grid */
    world->grid.cell_size = PHYS_GRID_CELL_SIZE;
    world->grid.origin = cm_vec3(
        -(PHYS_GRID_DIM * PHYS_GRID_CELL_SIZE) / 2.0f,
        -(PHYS_GRID_DIM * PHYS_GRID_CELL_SIZE) / 2.0f,
        -(PHYS_GRID_DIM * PHYS_GRID_CELL_SIZE) / 2.0f
    );
}

void phys_world_set_callback(PhysWorld *world, PhysCollisionCallback cb, void *userdata) {
    world->on_collision = cb;
    world->callback_userdata = userdata;
}

/* ================================================================== */
/* Body                                                                 */
/* ================================================================== */

PhysBody *phys_body_create(PhysWorld *world, CmVec3 pos, float mass, PhysCollider collider) {
    if (world->body_count >= PHYS_MAX_BODIES) return NULL;

    PhysBody *b = &world->bodies[world->body_count];
    memset(b, 0, sizeof(*b));
    b->position = pos;
    b->rotation = cm_quat_identity();
    b->mass = mass;
    b->inv_mass = mass > 0 ? (1.0f / mass) : 0;
    b->inertia = compute_inertia(mass, &collider);
    b->inv_inertia = b->inertia > 0 ? (1.0f / b->inertia) : 0;
    b->restitution = 0.3f;
    b->friction = 0.5f;
    b->linear_damping = 0.01f;
    b->angular_damping = 0.05f;
    b->collider = collider;
    b->active = true;
    b->is_static = (mass <= 0);
    b->collision_mask = 0xFFFFFFFF;
    b->collision_layer = 1;
    b->id = world->body_count;

    world->body_count++;
    return b;
}

PhysBody *phys_body_create_static(PhysWorld *world, CmVec3 pos, PhysCollider collider) {
    PhysBody *b = phys_body_create(world, pos, 0, collider);
    if (b) b->is_static = true;
    return b;
}

void phys_body_destroy(PhysWorld *world, PhysBody *body) {
    if (!body || !body->active) return;
    body->active = false;

    int idx = body->id;
    int last = world->body_count - 1;
    if (idx < last) {
        world->bodies[idx] = world->bodies[last];
        world->bodies[idx].id = idx;
    }
    world->body_count--;
}

void phys_body_apply_force(PhysBody *body, CmVec3 force) {
    body->force = cm_vec3_add(body->force, force);
}

void phys_body_apply_force_at(PhysBody *body, CmVec3 force, CmVec3 point) {
    body->force = cm_vec3_add(body->force, force);
    CmVec3 r = cm_vec3_sub(point, body->position);
    body->torque = cm_vec3_add(body->torque, cm_vec3_cross(r, force));
}

void phys_body_apply_impulse(PhysBody *body, CmVec3 impulse) {
    if (body->inv_mass <= 0) return;
    body->linear_velocity = cm_vec3_add(body->linear_velocity,
                                          cm_vec3_scale(impulse, body->inv_mass));
}

void phys_body_apply_impulse_at(PhysBody *body, CmVec3 impulse, CmVec3 point) {
    if (body->inv_mass <= 0) return;
    body->linear_velocity = cm_vec3_add(body->linear_velocity,
                                          cm_vec3_scale(impulse, body->inv_mass));
    CmVec3 r = cm_vec3_sub(point, body->position);
    CmVec3 angular_impulse = cm_vec3_cross(r, impulse);
    body->angular_velocity = cm_vec3_add(body->angular_velocity,
                                           cm_vec3_scale(angular_impulse, body->inv_inertia));
}

void phys_body_set_velocity(PhysBody *body, CmVec3 vel) {
    body->linear_velocity = vel;
}

/* ================================================================== */
/* Broadphase: Spatial Grid                                             */
/* ================================================================== */

static void grid_clear(PhysSpatialGrid *g) {
    for (int x = 0; x < PHYS_GRID_DIM; x++)
        for (int y = 0; y < PHYS_GRID_DIM; y++)
            for (int z = 0; z < PHYS_GRID_DIM; z++)
                g->cells[x][y][z].count = 0;
}

static void grid_coords(const PhysSpatialGrid *g, CmVec3 pos, int *gx, int *gy, int *gz) {
    *gx = (int)((pos.x - g->origin.x) / g->cell_size);
    *gy = (int)((pos.y - g->origin.y) / g->cell_size);
    *gz = (int)((pos.z - g->origin.z) / g->cell_size);
    /* clamp */
    if (*gx < 0) *gx = 0;
    if (*gx >= PHYS_GRID_DIM) *gx = PHYS_GRID_DIM - 1;
    if (*gy < 0) *gy = 0;
    if (*gy >= PHYS_GRID_DIM) *gy = PHYS_GRID_DIM - 1;
    if (*gz < 0) *gz = 0;
    if (*gz >= PHYS_GRID_DIM) *gz = PHYS_GRID_DIM - 1;
}

static void grid_insert(PhysSpatialGrid *g, int body_index, CmVec3 pos) {
    int gx, gy, gz;
    grid_coords(g, pos, &gx, &gy, &gz);
    PhysGridCell *cell = &g->cells[gx][gy][gz];
    if (cell->count < PHYS_GRID_MAX_PER_CELL) {
        cell->body_indices[cell->count++] = body_index;
    }
}

static void grid_build(PhysSpatialGrid *g, PhysBody *bodies, int count) {
    grid_clear(g);
    for (int i = 0; i < count; i++) {
        if (!bodies[i].active) continue;
        grid_insert(g, i, bodies[i].position);
    }
}

/* ================================================================== */
/* Narrowphase Collision Detection                                      */
/* ================================================================== */

static CmVec3 get_body_aabb_min(const PhysBody *b) {
    switch (b->collider.type) {
        case PHYS_SHAPE_SPHERE: {
            float r = b->collider.sphere.radius;
            return cm_vec3_sub(b->position, cm_vec3(r, r, r));
        }
        case PHYS_SHAPE_AABB:
            return cm_vec3_sub(b->position, b->collider.aabb.half_extents);
        case PHYS_SHAPE_CAPSULE: {
            float r = b->collider.capsule.radius;
            float hh = b->collider.capsule.half_height + r;
            return cm_vec3_sub(b->position, cm_vec3(r, hh, r));
        }
        case PHYS_SHAPE_PLANE:
            return cm_vec3(-1e10f, -1e10f, -1e10f);
    }
    return b->position;
}

static CmVec3 get_body_aabb_max(const PhysBody *b) {
    switch (b->collider.type) {
        case PHYS_SHAPE_SPHERE: {
            float r = b->collider.sphere.radius;
            return cm_vec3_add(b->position, cm_vec3(r, r, r));
        }
        case PHYS_SHAPE_AABB:
            return cm_vec3_add(b->position, b->collider.aabb.half_extents);
        case PHYS_SHAPE_CAPSULE: {
            float r = b->collider.capsule.radius;
            float hh = b->collider.capsule.half_height + r;
            return cm_vec3_add(b->position, cm_vec3(r, hh, r));
        }
        case PHYS_SHAPE_PLANE:
            return cm_vec3(1e10f, 1e10f, 1e10f);
    }
    return b->position;
}

/* Broadphase AABB overlap test */
static bool aabb_overlap(const PhysBody *a, const PhysBody *b) {
    CmVec3 amin = get_body_aabb_min(a), amax = get_body_aabb_max(a);
    CmVec3 bmin = get_body_aabb_min(b), bmax = get_body_aabb_max(b);
    return (amin.x <= bmax.x && amax.x >= bmin.x) &&
           (amin.y <= bmax.y && amax.y >= bmin.y) &&
           (amin.z <= bmax.z && amax.z >= bmin.z);
}

/* --- Sphere vs Sphere --- */
static bool collide_sphere_sphere(const PhysBody *a, const PhysBody *b, PhysContact *c) {
    CmVec3 d = cm_vec3_sub(b->position, a->position);
    float dist2 = cm_vec3_dot(d, d);
    float r_sum = a->collider.sphere.radius + b->collider.sphere.radius;
    if (dist2 >= r_sum * r_sum) return false;

    float dist = sqrtf(dist2);
    c->normal = dist > CM_EPSILON ? cm_vec3_scale(d, 1.0f / dist) : cm_vec3(0, 1, 0);
    c->depth = r_sum - dist;
    c->point = cm_vec3_add(a->position,
                 cm_vec3_scale(c->normal, a->collider.sphere.radius - c->depth * 0.5f));
    return true;
}

/* --- Sphere vs AABB --- */
static bool collide_sphere_aabb(const PhysBody *sphere, const PhysBody *box, PhysContact *c) {
    CmVec3 spos = sphere->position;
    CmVec3 bmin = cm_vec3_sub(box->position, box->collider.aabb.half_extents);
    CmVec3 bmax = cm_vec3_add(box->position, box->collider.aabb.half_extents);

    CmVec3 closest;
    closest.x = phys_clamp(spos.x, bmin.x, bmax.x);
    closest.y = phys_clamp(spos.y, bmin.y, bmax.y);
    closest.z = phys_clamp(spos.z, bmin.z, bmax.z);

    /* d points from closest on box toward sphere = from B toward A.
       We need normal from A (sphere) to B (box), so negate. */
    CmVec3 d = cm_vec3_sub(closest, spos);
    float dist2 = cm_vec3_dot(d, d);
    float r = sphere->collider.sphere.radius;

    if (dist2 >= r * r) return false;

    float dist = sqrtf(dist2);
    if (dist > CM_EPSILON) {
        c->normal = cm_vec3_scale(d, 1.0f / dist);
    } else {
        /* sphere center inside AABB — push out toward box center (from A to B) */
        float dx = phys_min(spos.x - bmin.x, bmax.x - spos.x);
        float dy = phys_min(spos.y - bmin.y, bmax.y - spos.y);
        float dz = phys_min(spos.z - bmin.z, bmax.z - spos.z);
        if (dx <= dy && dx <= dz) c->normal = spos.x - bmin.x < bmax.x - spos.x ? cm_vec3(1,0,0) : cm_vec3(-1,0,0);
        else if (dy <= dz)        c->normal = spos.y - bmin.y < bmax.y - spos.y ? cm_vec3(0,1,0) : cm_vec3(0,-1,0);
        else                       c->normal = spos.z - bmin.z < bmax.z - spos.z ? cm_vec3(0,0,1) : cm_vec3(0,0,-1);
    }
    c->depth = r - dist;
    c->point = closest;
    return true;
}

/* --- Sphere vs Plane --- */
static bool collide_sphere_plane(const PhysBody *sphere, const PhysBody *plane_b, PhysContact *c) {
    CmVec3 n = plane_b->collider.plane.normal;
    float d = plane_b->collider.plane.distance;
    float dist = cm_vec3_dot(sphere->position, n) - d;
    float r = sphere->collider.sphere.radius;

    if (dist >= r) return false;

    /* Normal from A (sphere) to B (plane) = opposite of plane normal */
    c->normal = cm_vec3_neg(n);
    c->depth = r - dist;
    c->point = cm_vec3_sub(sphere->position, cm_vec3_scale(n, dist));
    return true;
}

/* --- AABB vs AABB --- */
static bool collide_aabb_aabb(const PhysBody *a, const PhysBody *b, PhysContact *c) {
    CmVec3 d = cm_vec3_sub(b->position, a->position);
    CmVec3 ha = a->collider.aabb.half_extents;
    CmVec3 hb = b->collider.aabb.half_extents;

    float ox = (ha.x + hb.x) - fabsf(d.x);
    if (ox <= 0) return false;
    float oy = (ha.y + hb.y) - fabsf(d.y);
    if (oy <= 0) return false;
    float oz = (ha.z + hb.z) - fabsf(d.z);
    if (oz <= 0) return false;

    /* Resolve along minimum overlap axis */
    if (ox <= oy && ox <= oz) {
        c->normal = d.x > 0 ? cm_vec3(1,0,0) : cm_vec3(-1,0,0);
        c->depth = ox;
    } else if (oy <= oz) {
        c->normal = d.y > 0 ? cm_vec3(0,1,0) : cm_vec3(0,-1,0);
        c->depth = oy;
    } else {
        c->normal = d.z > 0 ? cm_vec3(0,0,1) : cm_vec3(0,0,-1);
        c->depth = oz;
    }
    c->point = cm_vec3_add(a->position, cm_vec3_scale(d, 0.5f));
    return true;
}

/* --- AABB vs Plane --- */
static bool collide_aabb_plane(const PhysBody *box, const PhysBody *plane_b, PhysContact *c) {
    CmVec3 n = plane_b->collider.plane.normal;
    float d = plane_b->collider.plane.distance;
    CmVec3 he = box->collider.aabb.half_extents;

    /* Projected extent of AABB onto plane normal */
    float extent = he.x * fabsf(n.x) + he.y * fabsf(n.y) + he.z * fabsf(n.z);
    float center_dist = cm_vec3_dot(box->position, n) - d;

    if (center_dist >= extent) return false;

    /* Normal from A (box) to B (plane) = opposite of plane normal */
    c->normal = cm_vec3_neg(n);
    c->depth = extent - center_dist;
    c->point = cm_vec3_sub(box->position, cm_vec3_scale(n, center_dist));
    return true;
}

/* --- Capsule helpers --- */
static void capsule_endpoints(const PhysBody *b, CmVec3 *p0, CmVec3 *p1) {
    float hh = b->collider.capsule.half_height;
    /* Capsule aligned along local Y - apply rotation */
    CmVec3 up = cm_vec3(0, hh, 0);
    /* Simple rotation of up vector by quaternion */
    CmQuat q = b->rotation;
    /* qvq* for rotating a vector */
    float qx = q.x, qy = q.y, qz = q.z, qw = q.w;
    float tx = 2*(qy*up.z - qz*up.y);
    float ty = 2*(qz*up.x - qx*up.z);
    float tz = 2*(qx*up.y - qy*up.x);
    CmVec3 rotated;
    rotated.x = up.x + qw*tx + (qy*tz - qz*ty);
    rotated.y = up.y + qw*ty + (qz*tx - qx*tz);
    rotated.z = up.z + qw*tz + (qx*ty - qy*tx);

    *p0 = cm_vec3_add(b->position, rotated);
    *p1 = cm_vec3_sub(b->position, rotated);
}

static CmVec3 closest_point_segment(CmVec3 a, CmVec3 b, CmVec3 p) {
    CmVec3 ab = cm_vec3_sub(b, a);
    float t = cm_vec3_dot(cm_vec3_sub(p, a), ab);
    float denom = cm_vec3_dot(ab, ab);
    if (denom < CM_EPSILON) return a;
    t = phys_clamp(t / denom, 0, 1);
    return cm_vec3_add(a, cm_vec3_scale(ab, t));
}

/* Find closest points between two segments */
static void closest_points_segments(CmVec3 p0, CmVec3 p1, CmVec3 q0, CmVec3 q1,
                                      CmVec3 *cp, CmVec3 *cq) {
    CmVec3 d1 = cm_vec3_sub(p1, p0);
    CmVec3 d2 = cm_vec3_sub(q1, q0);
    CmVec3 r  = cm_vec3_sub(p0, q0);

    float a = cm_vec3_dot(d1, d1);
    float e = cm_vec3_dot(d2, d2);
    float f = cm_vec3_dot(d2, r);

    float s, t;

    if (a <= CM_EPSILON && e <= CM_EPSILON) {
        *cp = p0; *cq = q0; return;
    }
    if (a <= CM_EPSILON) {
        s = 0; t = phys_clamp(f / e, 0, 1);
    } else {
        float c_ = cm_vec3_dot(d1, r);
        if (e <= CM_EPSILON) {
            t = 0; s = phys_clamp(-c_ / a, 0, 1);
        } else {
            float b_ = cm_vec3_dot(d1, d2);
            float denom = a * e - b_ * b_;
            s = denom != 0 ? phys_clamp((b_ * f - c_ * e) / denom, 0, 1) : 0;
            t = (b_ * s + f) / e;
            if (t < 0) { t = 0; s = phys_clamp(-c_ / a, 0, 1); }
            else if (t > 1) { t = 1; s = phys_clamp((b_ - c_) / a, 0, 1); }
        }
    }
    *cp = cm_vec3_add(p0, cm_vec3_scale(d1, s));
    *cq = cm_vec3_add(q0, cm_vec3_scale(d2, t));
}

/* --- Capsule vs Capsule --- */
static bool collide_capsule_capsule(const PhysBody *a, const PhysBody *b, PhysContact *c) {
    CmVec3 a0, a1, b0, b1;
    capsule_endpoints(a, &a0, &a1);
    capsule_endpoints(b, &b0, &b1);

    CmVec3 ca, cb;
    closest_points_segments(a0, a1, b0, b1, &ca, &cb);

    CmVec3 d = cm_vec3_sub(cb, ca);
    float dist2 = cm_vec3_dot(d, d);
    float r_sum = a->collider.capsule.radius + b->collider.capsule.radius;
    if (dist2 >= r_sum * r_sum) return false;

    float dist = sqrtf(dist2);
    c->normal = dist > CM_EPSILON ? cm_vec3_scale(d, 1.0f / dist) : cm_vec3(0, 1, 0);
    c->depth = r_sum - dist;
    c->point = cm_vec3_add(ca, cm_vec3_scale(c->normal, a->collider.capsule.radius));
    return true;
}

/* --- Capsule vs Sphere --- */
static bool collide_capsule_sphere(const PhysBody *cap, const PhysBody *sph, PhysContact *c) {
    CmVec3 c0, c1;
    capsule_endpoints(cap, &c0, &c1);
    CmVec3 closest = closest_point_segment(c0, c1, sph->position);

    CmVec3 d = cm_vec3_sub(sph->position, closest);
    float dist2 = cm_vec3_dot(d, d);
    float r_sum = cap->collider.capsule.radius + sph->collider.sphere.radius;
    if (dist2 >= r_sum * r_sum) return false;

    float dist = sqrtf(dist2);
    c->normal = dist > CM_EPSILON ? cm_vec3_scale(d, 1.0f / dist) : cm_vec3(0, 1, 0);
    c->depth = r_sum - dist;
    c->point = cm_vec3_add(closest, cm_vec3_scale(c->normal, cap->collider.capsule.radius));
    return true;
}

/* --- Capsule vs Plane --- */
static bool collide_capsule_plane(const PhysBody *cap, const PhysBody *plane_b, PhysContact *c) {
    CmVec3 c0, c1;
    capsule_endpoints(cap, &c0, &c1);
    CmVec3 n = plane_b->collider.plane.normal;
    float d = plane_b->collider.plane.distance;
    float r = cap->collider.capsule.radius;

    float d0 = cm_vec3_dot(c0, n) - d;
    float d1 = cm_vec3_dot(c1, n) - d;

    /* Use the endpoint closest to the plane */
    float dist = d0 < d1 ? d0 : d1;
    CmVec3 closest = d0 < d1 ? c0 : c1;

    if (dist >= r) return false;

    /* Normal from A (capsule) to B (plane) = opposite of plane normal */
    c->normal = cm_vec3_neg(n);
    c->depth = r - dist;
    c->point = cm_vec3_sub(closest, cm_vec3_scale(n, dist));
    return true;
}

/* --- Capsule vs AABB --- */
static bool collide_capsule_aabb(const PhysBody *cap, const PhysBody *box, PhysContact *c) {
    /* Approximate: find closest point on capsule segment to box center,
       then do sphere-AABB test with that point */
    CmVec3 c0, c1;
    capsule_endpoints(cap, &c0, &c1);
    CmVec3 seg_closest = closest_point_segment(c0, c1, box->position);

    /* Now treat as sphere at seg_closest with capsule radius vs AABB */
    CmVec3 bmin = cm_vec3_sub(box->position, box->collider.aabb.half_extents);
    CmVec3 bmax = cm_vec3_add(box->position, box->collider.aabb.half_extents);

    CmVec3 closest;
    closest.x = phys_clamp(seg_closest.x, bmin.x, bmax.x);
    closest.y = phys_clamp(seg_closest.y, bmin.y, bmax.y);
    closest.z = phys_clamp(seg_closest.z, bmin.z, bmax.z);

    /* d_vec from seg_closest toward box closest = from A toward B */
    CmVec3 d_vec = cm_vec3_sub(closest, seg_closest);
    float dist2 = cm_vec3_dot(d_vec, d_vec);
    float r = cap->collider.capsule.radius;

    if (dist2 >= r * r) return false;

    float dist = sqrtf(dist2);
    if (dist > CM_EPSILON) {
        c->normal = cm_vec3_scale(d_vec, 1.0f / dist);
    } else {
        c->normal = cm_vec3(0, 1, 0);
    }
    c->depth = r - dist;
    c->point = closest;
    return true;
}

/* ================================================================== */
/* Collision Dispatch                                                   */
/* ================================================================== */

static bool collide_pair(PhysBody *a, PhysBody *b, PhysContact *c) {
    PhysShapeType ta = a->collider.type;
    PhysShapeType tb = b->collider.type;

    /* Order so we can handle asymmetric pairs */
    #define PAIR(x, y) ((x)*4 + (y))

    switch (PAIR(ta, tb)) {
        case PAIR(PHYS_SHAPE_SPHERE, PHYS_SHAPE_SPHERE):
            return collide_sphere_sphere(a, b, c);
        case PAIR(PHYS_SHAPE_SPHERE, PHYS_SHAPE_AABB):
            return collide_sphere_aabb(a, b, c);
        case PAIR(PHYS_SHAPE_AABB, PHYS_SHAPE_SPHERE):
            if (collide_sphere_aabb(b, a, c)) {
                c->normal = cm_vec3_neg(c->normal);
                return true;
            }
            return false;
        case PAIR(PHYS_SHAPE_SPHERE, PHYS_SHAPE_PLANE):
            return collide_sphere_plane(a, b, c);
        case PAIR(PHYS_SHAPE_PLANE, PHYS_SHAPE_SPHERE):
            if (collide_sphere_plane(b, a, c)) {
                c->normal = cm_vec3_neg(c->normal);
                return true;
            }
            return false;
        case PAIR(PHYS_SHAPE_AABB, PHYS_SHAPE_AABB):
            return collide_aabb_aabb(a, b, c);
        case PAIR(PHYS_SHAPE_AABB, PHYS_SHAPE_PLANE):
            return collide_aabb_plane(a, b, c);
        case PAIR(PHYS_SHAPE_PLANE, PHYS_SHAPE_AABB):
            if (collide_aabb_plane(b, a, c)) {
                c->normal = cm_vec3_neg(c->normal);
                return true;
            }
            return false;
        case PAIR(PHYS_SHAPE_CAPSULE, PHYS_SHAPE_CAPSULE):
            return collide_capsule_capsule(a, b, c);
        case PAIR(PHYS_SHAPE_CAPSULE, PHYS_SHAPE_SPHERE):
            return collide_capsule_sphere(a, b, c);
        case PAIR(PHYS_SHAPE_SPHERE, PHYS_SHAPE_CAPSULE):
            if (collide_capsule_sphere(b, a, c)) {
                c->normal = cm_vec3_neg(c->normal);
                return true;
            }
            return false;
        case PAIR(PHYS_SHAPE_CAPSULE, PHYS_SHAPE_PLANE):
            return collide_capsule_plane(a, b, c);
        case PAIR(PHYS_SHAPE_PLANE, PHYS_SHAPE_CAPSULE):
            if (collide_capsule_plane(b, a, c)) {
                c->normal = cm_vec3_neg(c->normal);
                return true;
            }
            return false;
        case PAIR(PHYS_SHAPE_CAPSULE, PHYS_SHAPE_AABB):
            return collide_capsule_aabb(a, b, c);
        case PAIR(PHYS_SHAPE_AABB, PHYS_SHAPE_CAPSULE):
            if (collide_capsule_aabb(b, a, c)) {
                c->normal = cm_vec3_neg(c->normal);
                return true;
            }
            return false;
        default:
            return false; /* plane-plane etc */
    }
    #undef PAIR
}

/* ================================================================== */
/* Collision Detection Phase                                            */
/* ================================================================== */

static void detect_collisions(PhysWorld *world) {
    world->contact_count = 0;

    /* Build spatial grid */
    grid_build(&world->grid, world->bodies, world->body_count);

    /* Iterate grid cells, test pairs */
    for (int x = 0; x < PHYS_GRID_DIM; x++)
    for (int y = 0; y < PHYS_GRID_DIM; y++)
    for (int z = 0; z < PHYS_GRID_DIM; z++) {
        PhysGridCell *cell = &world->grid.cells[x][y][z];
        if (cell->count < 1) continue;

        /* Test pairs within this cell */
        for (int i = 0; i < cell->count; i++) {
            int ai = cell->body_indices[i];
            PhysBody *a = &world->bodies[ai];
            if (!a->active) continue;

            /* Against other bodies in same cell */
            for (int j = i + 1; j < cell->count; j++) {
                int bi = cell->body_indices[j];
                PhysBody *b = &world->bodies[bi];
                if (!b->active) continue;
                if (a->is_static && b->is_static) continue;

                /* Layer filter */
                if (!(a->collision_mask & b->collision_layer) ||
                    !(b->collision_mask & a->collision_layer)) continue;

                if (world->contact_count >= PHYS_MAX_CONTACTS) return;

                PhysContact *c = &world->contacts[world->contact_count];
                memset(c, 0, sizeof(*c));

                if (collide_pair(a, b, c)) {
                    c->body_a = a;
                    c->body_b = b;
                    world->contact_count++;

                    if (world->on_collision) {
                        world->on_collision(a, b, c, world->callback_userdata);
                    }
                }
            }

            /* Against adjacent cells */
            for (int dx = 0; dx <= 1; dx++)
            for (int dy = -1; dy <= 1; dy++)
            for (int dz = -1; dz <= 1; dz++) {
                if (dx == 0 && dy == 0 && dz == 0) continue;
                if (dx == 0 && (dy < 0 || (dy == 0 && dz <= 0))) continue;
                int nx = x+dx, ny = y+dy, nz = z+dz;
                if (nx < 0 || nx >= PHYS_GRID_DIM) continue;
                if (ny < 0 || ny >= PHYS_GRID_DIM) continue;
                if (nz < 0 || nz >= PHYS_GRID_DIM) continue;

                PhysGridCell *ncell = &world->grid.cells[nx][ny][nz];
                for (int j = 0; j < ncell->count; j++) {
                    int bi = ncell->body_indices[j];
                    PhysBody *b = &world->bodies[bi];
                    if (!b->active) continue;
                    if (a->is_static && b->is_static) continue;
                    if (!(a->collision_mask & b->collision_layer) ||
                        !(b->collision_mask & a->collision_layer)) continue;

                    if (world->contact_count >= PHYS_MAX_CONTACTS) return;

                    PhysContact *c = &world->contacts[world->contact_count];
                    memset(c, 0, sizeof(*c));

                    if (collide_pair(a, b, c)) {
                        c->body_a = a;
                        c->body_b = b;
                        world->contact_count++;

                        if (world->on_collision) {
                            world->on_collision(a, b, c, world->callback_userdata);
                        }
                    }
                }
            }
        }
    }

    /* Also test all bodies against plane bodies (planes are infinite, not in grid) */
    for (int i = 0; i < world->body_count; i++) {
        PhysBody *p = &world->bodies[i];
        if (!p->active || p->collider.type != PHYS_SHAPE_PLANE) continue;
        for (int j = 0; j < world->body_count; j++) {
            if (i == j) continue;
            PhysBody *b = &world->bodies[j];
            if (!b->active || b->collider.type == PHYS_SHAPE_PLANE) continue;
            if (!(p->collision_mask & b->collision_layer)) continue;

            if (world->contact_count >= PHYS_MAX_CONTACTS) return;

            PhysContact *c = &world->contacts[world->contact_count];
            memset(c, 0, sizeof(*c));

            if (collide_pair(b, p, c)) {
                c->body_a = b;
                c->body_b = p;
                world->contact_count++;

                if (world->on_collision) {
                    world->on_collision(b, p, c, world->callback_userdata);
                }
            }
        }
    }
}

/* ================================================================== */
/* Sequential Impulse Solver                                            */
/* ================================================================== */

static void solve_contacts(PhysWorld *world) {
    for (int iter = 0; iter < world->solver_iterations; iter++) {
        for (int i = 0; i < world->contact_count; i++) {
            PhysContact *c = &world->contacts[i];
            PhysBody *a = c->body_a;
            PhysBody *b = c->body_b;

            if (a->is_sensor || b->is_sensor) continue;

            float ima = a->inv_mass;
            float imb = b->inv_mass;
            float inv_mass_sum = ima + imb;
            if (inv_mass_sum <= 0) continue;

            /* Relative velocity at contact point */
            CmVec3 ra = cm_vec3_sub(c->point, a->position);
            CmVec3 rb = cm_vec3_sub(c->point, b->position);

            CmVec3 vel_a = cm_vec3_add(a->linear_velocity, cm_vec3_cross(a->angular_velocity, ra));
            CmVec3 vel_b = cm_vec3_add(b->linear_velocity, cm_vec3_cross(b->angular_velocity, rb));
            CmVec3 rel_vel = cm_vec3_sub(vel_b, vel_a);

            float vel_normal = cm_vec3_dot(rel_vel, c->normal);

            /* Skip if separating */
            if (vel_normal > 0) continue;

            /* Effective mass along normal */
            float rna = cm_vec3_dot(cm_vec3_cross(cm_vec3_scale(cm_vec3_cross(ra, c->normal), a->inv_inertia), ra), c->normal);
            float rnb = cm_vec3_dot(cm_vec3_cross(cm_vec3_scale(cm_vec3_cross(rb, c->normal), b->inv_inertia), rb), c->normal);
            float eff_mass = inv_mass_sum + rna + rnb;
            if (eff_mass <= 0) continue;

            /* Restitution */
            float e = phys_min(a->restitution, b->restitution);

            /* Baumgarte stabilization: push apart to fix penetration */
            float baumgarte = 0.2f;
            float slop = 0.005f;
            float bias = (baumgarte / PHYS_FIXED_DT) * phys_max(c->depth - slop, 0);

            /* Normal impulse */
            float j = -(1.0f + e) * vel_normal + bias;
            j /= eff_mass;

            /* Accumulate and clamp */
            float old_impulse = c->normal_impulse;
            c->normal_impulse = phys_max(old_impulse + j, 0);
            j = c->normal_impulse - old_impulse;

            CmVec3 impulse = cm_vec3_scale(c->normal, j);

            /* Apply normal impulse */
            a->linear_velocity = cm_vec3_sub(a->linear_velocity, cm_vec3_scale(impulse, ima));
            b->linear_velocity = cm_vec3_add(b->linear_velocity, cm_vec3_scale(impulse, imb));
            a->angular_velocity = cm_vec3_sub(a->angular_velocity,
                cm_vec3_scale(cm_vec3_cross(ra, impulse), a->inv_inertia));
            b->angular_velocity = cm_vec3_add(b->angular_velocity,
                cm_vec3_scale(cm_vec3_cross(rb, impulse), b->inv_inertia));

            /* --- Friction --- */
            /* Recalculate relative velocity */
            vel_a = cm_vec3_add(a->linear_velocity, cm_vec3_cross(a->angular_velocity, ra));
            vel_b = cm_vec3_add(b->linear_velocity, cm_vec3_cross(b->angular_velocity, rb));
            rel_vel = cm_vec3_sub(vel_b, vel_a);

            CmVec3 tangent = cm_vec3_sub(rel_vel, cm_vec3_scale(c->normal, cm_vec3_dot(rel_vel, c->normal)));
            float tangent_len = cm_vec3_length(tangent);
            if (tangent_len < CM_EPSILON) continue;
            tangent = cm_vec3_scale(tangent, 1.0f / tangent_len);

            float jt = -cm_vec3_dot(rel_vel, tangent) / eff_mass;

            /* Coulomb friction clamp */
            float mu = (a->friction + b->friction) * 0.5f;
            float max_friction = mu * c->normal_impulse;

            float old_t = c->tangent_impulse;
            c->tangent_impulse = phys_clamp(old_t + jt, -max_friction, max_friction);
            jt = c->tangent_impulse - old_t;

            CmVec3 friction_impulse = cm_vec3_scale(tangent, jt);

            a->linear_velocity = cm_vec3_sub(a->linear_velocity, cm_vec3_scale(friction_impulse, ima));
            b->linear_velocity = cm_vec3_add(b->linear_velocity, cm_vec3_scale(friction_impulse, imb));
            a->angular_velocity = cm_vec3_sub(a->angular_velocity,
                cm_vec3_scale(cm_vec3_cross(ra, friction_impulse), a->inv_inertia));
            b->angular_velocity = cm_vec3_add(b->angular_velocity,
                cm_vec3_scale(cm_vec3_cross(rb, friction_impulse), b->inv_inertia));
        }
    }
}

/* ================================================================== */
/* Constraint Solver                                                    */
/* ================================================================== */

static void solve_constraints(PhysWorld *world, float dt) {
    for (int i = 0; i < world->constraint_count; i++) {
        PhysConstraint *con = &world->constraints[i];
        if (!con->active) continue;

        PhysBody *a = con->body_a;
        PhysBody *b = con->body_b;

        switch (con->type) {
            case PHYS_CONSTRAINT_PIN: {
                /* Keep anchor_a (in local space of body A) at world position anchor_b */
                CmVec3 world_anchor = cm_vec3_add(a->position, con->anchor_a);
                CmVec3 target = b ? cm_vec3_add(b->position, con->anchor_b) : con->anchor_b;
                CmVec3 delta = cm_vec3_sub(target, world_anchor);

                float correction = 0.5f;
                if (a->inv_mass > 0)
                    a->position = cm_vec3_add(a->position, cm_vec3_scale(delta, correction));
                if (b && b->inv_mass > 0)
                    b->position = cm_vec3_sub(b->position, cm_vec3_scale(delta, correction));

                /* Velocity correction */
                CmVec3 vel_delta = cm_vec3_scale(delta, 1.0f / dt);
                if (a->inv_mass > 0) {
                    float factor = b ? 0.5f : 1.0f;
                    a->linear_velocity = cm_vec3_add(a->linear_velocity, cm_vec3_scale(vel_delta, factor));
                }
                if (b && b->inv_mass > 0) {
                    a->linear_velocity = cm_vec3_sub(b->linear_velocity, cm_vec3_scale(vel_delta, 0.5f));
                }
                break;
            }
            case PHYS_CONSTRAINT_DISTANCE: {
                CmVec3 pa = cm_vec3_add(a->position, con->anchor_a);
                CmVec3 pb = b ? cm_vec3_add(b->position, con->anchor_b) : con->anchor_b;
                CmVec3 delta = cm_vec3_sub(pb, pa);
                float dist = cm_vec3_length(delta);
                if (dist < CM_EPSILON) break;

                CmVec3 dir = cm_vec3_scale(delta, 1.0f / dist);
                float error = dist - con->distance;

                float ima = a->inv_mass;
                float imb = b ? b->inv_mass : 0;
                float total = ima + imb;
                if (total <= 0) break;

                CmVec3 correction = cm_vec3_scale(dir, error / total);
                if (a->inv_mass > 0)
                    a->position = cm_vec3_add(a->position, cm_vec3_scale(correction, ima));
                if (b && b->inv_mass > 0)
                    b->position = cm_vec3_sub(b->position, cm_vec3_scale(correction, imb));
                break;
            }
            case PHYS_CONSTRAINT_HINGE: {
                /* Simplified: maintain distance + restrict rotation to axis */
                CmVec3 pa = cm_vec3_add(a->position, con->anchor_a);
                CmVec3 pb = b ? cm_vec3_add(b->position, con->anchor_b) : con->anchor_b;
                CmVec3 delta = cm_vec3_sub(pb, pa);

                /* Position correction (like pin at anchors) */
                float corr_factor = 0.3f;
                float ima = a->inv_mass;
                float imb = b ? b->inv_mass : 0;
                float total = ima + imb;
                if (total > 0) {
                    CmVec3 correction = cm_vec3_scale(delta, corr_factor / total);
                    if (a->inv_mass > 0)
                        a->position = cm_vec3_add(a->position, cm_vec3_scale(correction, ima));
                    if (b && b->inv_mass > 0)
                        b->position = cm_vec3_sub(b->position, cm_vec3_scale(correction, imb));
                }

                /* Angular: project angular velocity onto hinge axis */
                CmVec3 axis = cm_vec3_normalize(con->axis);
                if (a->inv_inertia > 0) {
                    float along = cm_vec3_dot(a->angular_velocity, axis);
                    a->angular_velocity = cm_vec3_scale(axis, along);
                }
                break;
            }
        }
    }
}

/* ================================================================== */
/* Integration                                                          */
/* ================================================================== */

static void integrate(PhysWorld *world, float dt) {
    for (int i = 0; i < world->body_count; i++) {
        PhysBody *b = &world->bodies[i];
        if (!b->active || b->is_static) continue;

        /* Apply gravity */
        CmVec3 accel = cm_vec3_add(
            world->gravity,
            cm_vec3_scale(b->force, b->inv_mass)
        );

        /* Semi-implicit Euler */
        b->linear_velocity = cm_vec3_add(b->linear_velocity, cm_vec3_scale(accel, dt));
        b->angular_velocity = cm_vec3_add(b->angular_velocity,
                                            cm_vec3_scale(b->torque, b->inv_inertia * dt));

        /* Damping */
        b->linear_velocity = cm_vec3_scale(b->linear_velocity, 1.0f - b->linear_damping);
        b->angular_velocity = cm_vec3_scale(b->angular_velocity, 1.0f - b->angular_damping);

        /* Update position + rotation */
        b->position = cm_vec3_add(b->position, cm_vec3_scale(b->linear_velocity, dt));

        /* Integrate rotation (quaternion) */
        float wx = b->angular_velocity.x;
        float wy = b->angular_velocity.y;
        float wz = b->angular_velocity.z;
        CmQuat spin;
        spin.x = 0.5f * dt * (wx * b->rotation.w + wy * b->rotation.z - wz * b->rotation.y);
        spin.y = 0.5f * dt * (wy * b->rotation.w + wz * b->rotation.x - wx * b->rotation.z);
        spin.z = 0.5f * dt * (wz * b->rotation.w + wx * b->rotation.y - wy * b->rotation.x);
        spin.w = 0.5f * dt * (-wx * b->rotation.x - wy * b->rotation.y - wz * b->rotation.z);

        b->rotation.x += spin.x;
        b->rotation.y += spin.y;
        b->rotation.z += spin.z;
        b->rotation.w += spin.w;
        b->rotation = cm_quat_normalize(b->rotation);

        /* Clear forces */
        b->force = cm_vec3(0, 0, 0);
        b->torque = cm_vec3(0, 0, 0);
    }
}

/* Positional correction to resolve remaining penetration */
static void positional_correction(PhysWorld *world) {
    for (int i = 0; i < world->contact_count; i++) {
        PhysContact *c = &world->contacts[i];
        PhysBody *a = c->body_a;
        PhysBody *b = c->body_b;
        if (a->is_sensor || b->is_sensor) continue;

        float ima = a->inv_mass;
        float imb = b->inv_mass;
        float total = ima + imb;
        if (total <= 0) continue;

        float percent = 0.4f;
        float slop = 0.005f;
        float correction_mag = phys_max(c->depth - slop, 0) / total * percent;
        CmVec3 correction = cm_vec3_scale(c->normal, correction_mag);

        if (ima > 0) a->position = cm_vec3_sub(a->position, cm_vec3_scale(correction, ima));
        if (imb > 0) b->position = cm_vec3_add(b->position, cm_vec3_scale(correction, imb));
    }
}

/* ================================================================== */
/* World Step                                                           */
/* ================================================================== */

void phys_world_step(PhysWorld *world, float dt) {
    (void)dt; /* always use fixed timestep internally */

    /* 1. Integrate forces & velocity */
    integrate(world, PHYS_FIXED_DT);

    /* 2. Broadphase + narrowphase collision detection */
    detect_collisions(world);

    /* 3. Solve contact constraints */
    solve_contacts(world);

    /* 4. Solve user constraints */
    solve_constraints(world, PHYS_FIXED_DT);

    /* 5. Positional correction */
    positional_correction(world);
}

/* ================================================================== */
/* Constraint API                                                       */
/* ================================================================== */

PhysConstraint *phys_constraint_pin(PhysWorld *world, PhysBody *body,
                                      CmVec3 local_anchor, CmVec3 world_anchor) {
    if (world->constraint_count >= PHYS_MAX_CONSTRAINTS) return NULL;
    PhysConstraint *c = &world->constraints[world->constraint_count++];
    memset(c, 0, sizeof(*c));
    c->type = PHYS_CONSTRAINT_PIN;
    c->body_a = body;
    c->body_b = NULL;
    c->anchor_a = local_anchor;
    c->anchor_b = world_anchor;
    c->active = true;
    return c;
}

PhysConstraint *phys_constraint_distance(PhysWorld *world,
                                           PhysBody *a, CmVec3 anchor_a,
                                           PhysBody *b, CmVec3 anchor_b,
                                           float distance) {
    if (world->constraint_count >= PHYS_MAX_CONSTRAINTS) return NULL;
    PhysConstraint *c = &world->constraints[world->constraint_count++];
    memset(c, 0, sizeof(*c));
    c->type = PHYS_CONSTRAINT_DISTANCE;
    c->body_a = a;
    c->body_b = b;
    c->anchor_a = anchor_a;
    c->anchor_b = anchor_b;
    c->distance = distance;
    c->active = true;
    return c;
}

PhysConstraint *phys_constraint_hinge(PhysWorld *world,
                                        PhysBody *a, CmVec3 anchor_a,
                                        PhysBody *b, CmVec3 anchor_b,
                                        CmVec3 axis) {
    if (world->constraint_count >= PHYS_MAX_CONSTRAINTS) return NULL;
    PhysConstraint *c = &world->constraints[world->constraint_count++];
    memset(c, 0, sizeof(*c));
    c->type = PHYS_CONSTRAINT_HINGE;
    c->body_a = a;
    c->body_b = b;
    c->anchor_a = anchor_a;
    c->anchor_b = anchor_b;
    c->axis = axis;
    c->active = true;
    return c;
}

/* ================================================================== */
/* Projectile                                                           */
/* ================================================================== */

PhysBody *phys_projectile_launch(PhysWorld *world, const PhysProjectile *proj) {
    PhysCollider col;
    col.type = PHYS_SHAPE_SPHERE;
    col.sphere.radius = proj->radius;

    PhysBody *body = phys_body_create(world, proj->position, proj->mass, col);
    if (!body) return NULL;

    body->linear_velocity = proj->velocity;
    body->angular_velocity = proj->spin;
    body->restitution = 0.6f;
    body->linear_damping = 0.0f;  /* we apply drag manually */
    return body;
}

void phys_projectile_apply_forces(PhysBody *body, const PhysProjectile *proj) {
    CmVec3 vel = body->linear_velocity;
    float speed = cm_vec3_length(vel);
    if (speed < CM_EPSILON) return;

    CmVec3 dir = cm_vec3_scale(vel, 1.0f / speed);

    /* Quadratic drag: F_drag = -0.5 * Cd * A * rho * v^2 * dir
       Simplified: F_drag = -drag_coeff * v^2 * dir */
    float drag_force = proj->drag_coeff * speed * speed;
    CmVec3 f_drag = cm_vec3_scale(dir, -drag_force);

    /* Magnus effect: F_magnus = magnus_coeff * (spin x vel) */
    CmVec3 f_magnus = cm_vec3_scale(cm_vec3_cross(proj->spin, vel), proj->magnus_coeff);

    /* Lift (optional) */
    CmVec3 f_lift = cm_vec3(0, 0, 0);
    if (proj->lift_coeff > 0) {
        CmVec3 up = cm_vec3(0, 1, 0);
        CmVec3 lift_dir = cm_vec3_cross(dir, cm_vec3_cross(up, dir));
        float lift_len = cm_vec3_length(lift_dir);
        if (lift_len > CM_EPSILON) {
            lift_dir = cm_vec3_scale(lift_dir, 1.0f / lift_len);
            f_lift = cm_vec3_scale(lift_dir, proj->lift_coeff * speed * speed);
        }
    }

    CmVec3 total = cm_vec3_add(cm_vec3_add(f_drag, f_magnus), f_lift);
    phys_body_apply_force(body, total);
}

/* ================================================================== */
/* Buoyancy                                                             */
/* ================================================================== */

void phys_apply_buoyancy(PhysBody *body, float water_y, float water_density) {
    if (body->inv_mass <= 0) return;

    float submerged = 0;
    float body_radius = 0;

    switch (body->collider.type) {
        case PHYS_SHAPE_SPHERE:
            body_radius = body->collider.sphere.radius;
            break;
        case PHYS_SHAPE_AABB:
            body_radius = body->collider.aabb.half_extents.y;
            break;
        case PHYS_SHAPE_CAPSULE:
            body_radius = body->collider.capsule.half_height + body->collider.capsule.radius;
            break;
        default:
            return;
    }

    float bottom = body->position.y - body_radius;
    float top = body->position.y + body_radius;

    if (bottom >= water_y) return; /* above water */

    if (top <= water_y) {
        submerged = 1.0f; /* fully submerged */
    } else {
        submerged = (water_y - bottom) / (top - bottom);
    }

    /* Buoyancy force = rho * V * g (approx using submerged fraction * body volume) */
    float volume = (4.0f / 3.0f) * CM_PI * body_radius * body_radius * body_radius;
    float buoyancy = water_density * volume * submerged * 9.81f;

    phys_body_apply_force(body, cm_vec3(0, buoyancy, 0));

    /* Water drag */
    float drag = 0.5f * submerged;
    body->linear_velocity = cm_vec3_scale(body->linear_velocity, 1.0f - drag * PHYS_FIXED_DT);
    body->angular_velocity = cm_vec3_scale(body->angular_velocity, 1.0f - drag * 0.5f * PHYS_FIXED_DT);
}

/* ================================================================== */
/* Character Controller                                                 */
/* ================================================================== */

PhysCharController phys_char_create(PhysWorld *world, CmVec3 pos,
                                      float radius, float height, float mass) {
    PhysCollider col;
    col.type = PHYS_SHAPE_CAPSULE;
    col.capsule.radius = radius;
    col.capsule.half_height = height * 0.5f;

    PhysCharController ctrl;
    memset(&ctrl, 0, sizeof(ctrl));
    ctrl.body = phys_body_create(world, pos, mass, col);
    if (ctrl.body) {
        ctrl.body->angular_damping = 0.99f; /* prevent spinning */
        ctrl.body->linear_damping = 0.05f;
    }
    ctrl.move_speed = 5.0f;
    ctrl.jump_force = 6.0f;
    ctrl.slope_limit = CM_DEG2RAD * 45.0f;
    return ctrl;
}

void phys_char_move(PhysCharController *ctrl, CmVec3 input_dir, float dt) {
    if (!ctrl->body) return;
    (void)dt;

    float len = cm_vec3_length(input_dir);
    if (len > CM_EPSILON) {
        CmVec3 dir = cm_vec3_scale(input_dir, 1.0f / len);
        CmVec3 target_vel = cm_vec3_scale(dir, ctrl->move_speed);
        /* Set horizontal velocity, preserve vertical */
        ctrl->body->linear_velocity.x = target_vel.x;
        ctrl->body->linear_velocity.z = target_vel.z;
    } else if (ctrl->grounded) {
        /* Decelerate on ground */
        ctrl->body->linear_velocity.x *= 0.85f;
        ctrl->body->linear_velocity.z *= 0.85f;
    }

    /* Keep character upright — zero angular velocity */
    ctrl->body->angular_velocity = cm_vec3(0, 0, 0);
}

void phys_char_jump(PhysCharController *ctrl) {
    if (!ctrl->body || !ctrl->grounded) return;
    ctrl->body->linear_velocity.y = ctrl->jump_force;
    ctrl->grounded = false;
}

void phys_char_update(PhysCharController *ctrl, PhysWorld *world, float dt) {
    if (!ctrl->body) return;
    (void)dt;

    /* Ground check: raycast down from feet */
    CmRay ray;
    ray.origin = ctrl->body->position;
    ray.direction = cm_vec3(0, -1, 0);

    float check_dist = ctrl->body->collider.capsule.half_height +
                        ctrl->body->collider.capsule.radius + 0.1f;

    PhysRayResult result = phys_raycast(world, ray, check_dist, 0xFFFFFFFF);
    ctrl->grounded = result.hit;
    if (result.hit) {
        ctrl->ground_normal = result.normal;

        /* Check slope */
        float slope_angle = acosf(phys_clamp(cm_vec3_dot(result.normal, cm_vec3(0, 1, 0)), -1, 1));
        if (slope_angle > ctrl->slope_limit) {
            /* Slide down steep slope */
            CmVec3 slide = cm_vec3_sub(result.normal,
                cm_vec3_scale(cm_vec3(0, 1, 0), cm_vec3_dot(result.normal, cm_vec3(0, 1, 0))));
            float slide_len = cm_vec3_length(slide);
            if (slide_len > CM_EPSILON) {
                slide = cm_vec3_scale(slide, 5.0f / slide_len);
                phys_body_apply_force(ctrl->body, cm_vec3_scale(slide, ctrl->body->mass));
            }
        }
    }
}

/* ================================================================== */
/* Raycasting                                                           */
/* ================================================================== */

static bool ray_vs_body(CmRay ray, const PhysBody *body, float *out_t, CmVec3 *out_normal) {
    switch (body->collider.type) {
        case PHYS_SHAPE_SPHERE: {
            CmSphere s;
            s.center = body->position;
            s.radius = body->collider.sphere.radius;
            float t;
            if (cm_ray_vs_sphere(ray, s, &t)) {
                *out_t = t;
                /* Normal at hit point */
                CmVec3 hit = cm_vec3_add(ray.origin, cm_vec3_scale(ray.direction, t));
                *out_normal = cm_vec3_normalize(cm_vec3_sub(hit, s.center));
                return true;
            }
            return false;
        }
        case PHYS_SHAPE_AABB: {
            CmAABB aabb;
            aabb.min = cm_vec3_sub(body->position, body->collider.aabb.half_extents);
            aabb.max = cm_vec3_add(body->position, body->collider.aabb.half_extents);
            float t;
            if (cm_ray_vs_aabb(ray, aabb, &t)) {
                *out_t = t;
                /* Approximate normal from hit point relative to AABB center */
                CmVec3 hit = cm_vec3_add(ray.origin, cm_vec3_scale(ray.direction, t));
                CmVec3 rel = cm_vec3_sub(hit, body->position);
                CmVec3 he = body->collider.aabb.half_extents;
                float ax = fabsf(rel.x / he.x);
                float ay = fabsf(rel.y / he.y);
                float az = fabsf(rel.z / he.z);
                if (ax > ay && ax > az)
                    *out_normal = cm_vec3(rel.x > 0 ? 1.0f : -1.0f, 0, 0);
                else if (ay > az)
                    *out_normal = cm_vec3(0, rel.y > 0 ? 1.0f : -1.0f, 0);
                else
                    *out_normal = cm_vec3(0, 0, rel.z > 0 ? 1.0f : -1.0f);
                return true;
            }
            return false;
        }
        case PHYS_SHAPE_PLANE: {
            CmPlane p;
            p.normal = body->collider.plane.normal;
            p.distance = body->collider.plane.distance;
            float t;
            if (cm_ray_vs_plane(ray, p, &t)) {
                *out_t = t;
                *out_normal = p.normal;
                return true;
            }
            return false;
        }
        case PHYS_SHAPE_CAPSULE: {
            /* Approximate: ray vs sphere at both endpoints + ray vs AABB body */
            CmVec3 c0, c1;
            float hh = body->collider.capsule.half_height;
            CmVec3 up = cm_vec3(0, hh, 0);
            c0 = cm_vec3_add(body->position, up);
            c1 = cm_vec3_sub(body->position, up);
            float r = body->collider.capsule.radius;

            float best_t = 1e30f;
            CmVec3 best_n = cm_vec3(0, 1, 0);
            bool hit = false;
            float t;

            CmSphere s0 = {c0, r};
            CmSphere s1 = {c1, r};
            if (cm_ray_vs_sphere(ray, s0, &t) && t < best_t) {
                best_t = t;
                CmVec3 hp = cm_vec3_add(ray.origin, cm_vec3_scale(ray.direction, t));
                best_n = cm_vec3_normalize(cm_vec3_sub(hp, c0));
                hit = true;
            }
            if (cm_ray_vs_sphere(ray, s1, &t) && t < best_t) {
                best_t = t;
                CmVec3 hp = cm_vec3_add(ray.origin, cm_vec3_scale(ray.direction, t));
                best_n = cm_vec3_normalize(cm_vec3_sub(hp, c1));
                hit = true;
            }
            /* Cylinder check: approximate with AABB */
            CmAABB cap_aabb;
            cap_aabb.min = cm_vec3_sub(body->position, cm_vec3(r, hh + r, r));
            cap_aabb.max = cm_vec3_add(body->position, cm_vec3(r, hh + r, r));
            if (cm_ray_vs_aabb(ray, cap_aabb, &t) && t < best_t) {
                best_t = t;
                best_n = cm_vec3(0, 1, 0); /* rough normal */
                hit = true;
            }

            if (hit) { *out_t = best_t; *out_normal = best_n; }
            return hit;
        }
    }
    return false;
}

PhysRayResult phys_raycast(const PhysWorld *world, CmRay ray, float max_dist,
                            uint32_t layer_mask) {
    PhysRayResult result;
    memset(&result, 0, sizeof(result));
    float closest = max_dist;

    for (int i = 0; i < world->body_count; i++) {
        const PhysBody *b = &world->bodies[i];
        if (!b->active) continue;
        if (!(layer_mask & b->collision_layer)) continue;

        float t;
        CmVec3 normal;
        if (ray_vs_body(ray, b, &t, &normal) && t < closest && t >= 0) {
            closest = t;
            result.hit = true;
            result.body = (PhysBody *)b;
            result.distance = t;
            result.normal = normal;
            result.point = cm_vec3_add(ray.origin, cm_vec3_scale(ray.direction, t));
        }
    }
    return result;
}
