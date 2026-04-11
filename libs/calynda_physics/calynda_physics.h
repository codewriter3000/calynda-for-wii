/*
 * calynda_physics.h — Lightweight 3D physics for Calynda Wii.
 *
 * Designed for ~200 bodies at 60fps on Wii hardware.
 * Fixed 1/60s timestep matching vsync.
 *
 * Features:
 * - Rigid body dynamics with semi-implicit Euler integration
 * - Collision detection: sphere/AABB/capsule/plane pairwise tests
 * - Broadphase via uniform spatial grid
 * - Sequential impulse solver with friction + restitution
 * - Constraints: pin, distance, hinge
 * - Game helpers: projectile with Magnus effect, buoyancy, character controller
 */

#ifndef CALYNDA_PHYSICS_H
#define CALYNDA_PHYSICS_H

#include "../calynda_math/calynda_math.h"

#include <stdbool.h>
#include <stdint.h>

/* ================================================================== */
/* Configuration                                                        */
/* ================================================================== */

#define PHYS_MAX_BODIES       256
#define PHYS_MAX_CONSTRAINTS   64
#define PHYS_MAX_CONTACTS     512
#define PHYS_FIXED_DT         (1.0f / 60.0f)
#define PHYS_GRAVITY          cm_vec3(0, -9.81f, 0)

#define PHYS_GRID_CELL_SIZE   4.0f
#define PHYS_GRID_DIM         32
#define PHYS_GRID_MAX_PER_CELL 16

/* ================================================================== */
/* Collider Shapes                                                      */
/* ================================================================== */

typedef enum {
    PHYS_SHAPE_SPHERE,
    PHYS_SHAPE_AABB,
    PHYS_SHAPE_CAPSULE,
    PHYS_SHAPE_PLANE       /* infinite ground plane */
} PhysShapeType;

typedef struct {
    PhysShapeType type;
    union {
        struct { float radius; } sphere;
        struct { CmVec3 half_extents; } aabb;
        struct { float radius; float half_height; } capsule;  /* along Y axis */
        struct { CmVec3 normal; float distance; } plane;
    };
} PhysCollider;

/* ================================================================== */
/* Rigid Body                                                           */
/* ================================================================== */

typedef struct {
    /* State */
    CmVec3  position;
    CmQuat  rotation;
    CmVec3  linear_velocity;
    CmVec3  angular_velocity;

    /* Properties */
    float   mass;             /* 0 = static/infinite mass */
    float   inv_mass;
    float   inertia;          /* simplified scalar inertia */
    float   inv_inertia;
    float   restitution;      /* bounciness (0..1) */
    float   friction;         /* surface friction (0..1) */
    float   linear_damping;   /* velocity drag (0..1) */
    float   angular_damping;

    /* Force accumulation (reset each step) */
    CmVec3  force;
    CmVec3  torque;

    /* Collider */
    PhysCollider collider;

    /* Metadata */
    bool    active;
    bool    is_static;        /* mass==0 but still participates in collision */
    bool    is_sensor;        /* triggers events but no response */
    int     id;              /* index in world body array */
    void   *userdata;
    uint32_t collision_mask;  /* bitmask for filtering */
    uint32_t collision_layer;
} PhysBody;

/* ================================================================== */
/* Contact                                                              */
/* ================================================================== */

typedef struct {
    PhysBody *body_a;
    PhysBody *body_b;
    CmVec3    point;
    CmVec3    normal;        /* from A to B */
    float     depth;
    float     normal_impulse;  /* accumulated for solver */
    float     tangent_impulse;
} PhysContact;

/* ================================================================== */
/* Constraints                                                          */
/* ================================================================== */

typedef enum {
    PHYS_CONSTRAINT_PIN,
    PHYS_CONSTRAINT_DISTANCE,
    PHYS_CONSTRAINT_HINGE
} PhysConstraintType;

typedef struct {
    PhysConstraintType type;
    PhysBody *body_a;
    PhysBody *body_b;       /* NULL = world anchor */
    CmVec3    anchor_a;     /* local-space anchor on A */
    CmVec3    anchor_b;     /* local-space anchor on B (or world pos if B==NULL) */
    float     distance;     /* for distance constraint */
    CmVec3    axis;         /* for hinge: rotation axis (local to A) */
    bool      active;
} PhysConstraint;

/* ================================================================== */
/* Spatial Grid (broadphase)                                            */
/* ================================================================== */

typedef struct {
    int body_indices[PHYS_GRID_MAX_PER_CELL];
    int count;
} PhysGridCell;

typedef struct {
    PhysGridCell cells[PHYS_GRID_DIM][PHYS_GRID_DIM][PHYS_GRID_DIM];
    float        cell_size;
    CmVec3       origin;    /* grid world-space origin (min corner) */
} PhysSpatialGrid;

/* ================================================================== */
/* Physics World                                                        */
/* ================================================================== */

/* Collision callback */
typedef void (*PhysCollisionCallback)(PhysBody *a, PhysBody *b,
                                       const PhysContact *contact,
                                       void *userdata);

typedef struct {
    PhysBody       bodies[PHYS_MAX_BODIES];
    int            body_count;

    PhysConstraint constraints[PHYS_MAX_CONSTRAINTS];
    int            constraint_count;

    PhysContact    contacts[PHYS_MAX_CONTACTS];
    int            contact_count;

    CmVec3         gravity;
    int            solver_iterations;

    PhysSpatialGrid grid;

    /* Callbacks */
    PhysCollisionCallback on_collision;
    void                 *callback_userdata;

    /* Water plane for buoyancy */
    bool   water_enabled;
    float  water_y;
    float  water_density;
} PhysWorld;

/* ================================================================== */
/* World API                                                            */
/* ================================================================== */

void phys_world_init(PhysWorld *world);

/* Step the physics simulation by one fixed timestep (1/60s) */
void phys_world_step(PhysWorld *world, float dt);

/* Set collision callback */
void phys_world_set_callback(PhysWorld *world, PhysCollisionCallback cb, void *userdata);

/* ================================================================== */
/* Body API                                                             */
/* ================================================================== */

/* Create a dynamic body. Returns body pointer, or NULL if pool full. */
PhysBody *phys_body_create(PhysWorld *world, CmVec3 pos, float mass, PhysCollider collider);

/* Create a static body (infinite mass). */
PhysBody *phys_body_create_static(PhysWorld *world, CmVec3 pos, PhysCollider collider);

/* Remove a body from the world */
void phys_body_destroy(PhysWorld *world, PhysBody *body);

/* Apply a force at the center of mass (accumulated, applied next step) */
void phys_body_apply_force(PhysBody *body, CmVec3 force);

/* Apply a force at a world-space point (generates torque) */
void phys_body_apply_force_at(PhysBody *body, CmVec3 force, CmVec3 point);

/* Apply an instantaneous impulse */
void phys_body_apply_impulse(PhysBody *body, CmVec3 impulse);

/* Apply an instantaneous impulse at a point */
void phys_body_apply_impulse_at(PhysBody *body, CmVec3 impulse, CmVec3 point);

/* Set body velocity directly */
void phys_body_set_velocity(PhysBody *body, CmVec3 vel);

/* ================================================================== */
/* Constraint API                                                       */
/* ================================================================== */

/* Pin constraint: keeps body.anchor_a at a fixed world position */
PhysConstraint *phys_constraint_pin(PhysWorld *world, PhysBody *body,
                                      CmVec3 local_anchor, CmVec3 world_anchor);

/* Distance constraint between two bodies */
PhysConstraint *phys_constraint_distance(PhysWorld *world,
                                           PhysBody *a, CmVec3 anchor_a,
                                           PhysBody *b, CmVec3 anchor_b,
                                           float distance);

/* Hinge constraint: bodies rotate around shared axis */
PhysConstraint *phys_constraint_hinge(PhysWorld *world,
                                        PhysBody *a, CmVec3 anchor_a,
                                        PhysBody *b, CmVec3 anchor_b,
                                        CmVec3 axis);

/* ================================================================== */
/* Game Helpers                                                         */
/* ================================================================== */

/* Projectile config with spin */
typedef struct {
    CmVec3  position;
    CmVec3  velocity;
    CmVec3  spin;          /* angular velocity (rad/s) — for Magnus effect */
    float   mass;
    float   radius;
    float   drag_coeff;    /* quadratic drag coefficient */
    float   magnus_coeff;  /* Magnus force multiplier */
    float   lift_coeff;    /* optional lift (frisbee) */
} PhysProjectile;

/* Launch a projectile body with drag + Magnus effect */
PhysBody *phys_projectile_launch(PhysWorld *world, const PhysProjectile *proj);

/* Apply projectile forces (call each physics step for the body) */
void phys_projectile_apply_forces(PhysBody *body, const PhysProjectile *proj);

/* Buoyancy: apply upward force on bodies below water_y */
void phys_apply_buoyancy(PhysBody *body, float water_y, float water_density);

/* ================================================================== */
/* Character Controller                                                 */
/* ================================================================== */

typedef struct {
    PhysBody   *body;          /* capsule body */
    float       move_speed;
    float       jump_force;
    bool        grounded;
    CmVec3      ground_normal;
    float       slope_limit;   /* max walkable slope angle (radians) */
} PhysCharController;

PhysCharController phys_char_create(PhysWorld *world, CmVec3 pos,
                                      float radius, float height, float mass);
void phys_char_move(PhysCharController *ctrl, CmVec3 input_dir, float dt);
void phys_char_jump(PhysCharController *ctrl);
void phys_char_update(PhysCharController *ctrl, PhysWorld *world, float dt);

/* ================================================================== */
/* Raycasting                                                           */
/* ================================================================== */

typedef struct {
    bool        hit;
    PhysBody   *body;
    CmVec3      point;
    CmVec3      normal;
    float       distance;
} PhysRayResult;

PhysRayResult phys_raycast(const PhysWorld *world, CmRay ray, float max_dist,
                            uint32_t layer_mask);

#endif /* CALYNDA_PHYSICS_H */
