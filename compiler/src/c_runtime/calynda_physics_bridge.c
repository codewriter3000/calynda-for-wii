/*
 * calynda_physics_bridge.c — Physics bridge: import game.physics;
 *
 * Callable kind range: 400–499
 */

#include "calynda_physics_bridge.h"

#include <stdio.h>
#include <string.h>

#ifdef CALYNDA_WII_BUILD
#include "calynda_physics.h"
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

CalyndaRtPackage __calynda_pkg_physics = {
    { CALYNDA_RT_OBJECT_MAGIC, CALYNDA_RT_OBJECT_PACKAGE },
    "physics"
};

/* ------------------------------------------------------------------ */
/* Callable IDs                                                         */
/* ------------------------------------------------------------------ */

enum {
    PHYS_CALL_WORLD_INIT = 400,
    PHYS_CALL_WORLD_STEP,
    PHYS_CALL_SET_GRAVITY,

    /* Body */
    PHYS_CALL_BODY_CREATE,
    PHYS_CALL_BODY_CREATE_STATIC,
    PHYS_CALL_BODY_DESTROY,
    PHYS_CALL_BODY_SET_POS,
    PHYS_CALL_BODY_GET_POS_X,
    PHYS_CALL_BODY_GET_POS_Y,
    PHYS_CALL_BODY_GET_POS_Z,
    PHYS_CALL_BODY_APPLY_FORCE,
    PHYS_CALL_BODY_APPLY_IMPULSE,
    PHYS_CALL_BODY_SET_VELOCITY,
    PHYS_CALL_BODY_SET_RESTITUTION,
    PHYS_CALL_BODY_SET_FRICTION,

    /* Shapes */
    PHYS_CALL_SHAPE_SPHERE,
    PHYS_CALL_SHAPE_BOX,
    PHYS_CALL_SHAPE_CAPSULE,
    PHYS_CALL_SHAPE_PLANE,

    /* Raycast */
    PHYS_CALL_RAYCAST,
    PHYS_CALL_RAY_HIT,
    PHYS_CALL_RAY_DISTANCE,

    /* Projectile */
    PHYS_CALL_PROJECTILE_LAUNCH,

    /* Character */
    PHYS_CALL_CHAR_CREATE,
    PHYS_CALL_CHAR_MOVE,
    PHYS_CALL_CHAR_JUMP,
    PHYS_CALL_CHAR_UPDATE,
    PHYS_CALL_CHAR_GROUNDED,
};

/* ------------------------------------------------------------------ */
/* Callable Objects                                                     */
/* ------------------------------------------------------------------ */

#define PHYS_CALLABLE(var, id, label) \
    static CalyndaRtExternCallable var = { \
        { CALYNDA_RT_OBJECT_MAGIC, CALYNDA_RT_OBJECT_EXTERN_CALLABLE }, \
        (CalyndaRtExternCallableKind)(id), \
        label \
    }

PHYS_CALLABLE(PHYS_WORLD_INIT_CALLABLE,   PHYS_CALL_WORLD_INIT,      "world_init");
PHYS_CALLABLE(PHYS_WORLD_STEP_CALLABLE,   PHYS_CALL_WORLD_STEP,      "world_step");
PHYS_CALLABLE(PHYS_SET_GRAVITY_CALLABLE,  PHYS_CALL_SET_GRAVITY,     "set_gravity");
PHYS_CALLABLE(PHYS_BODY_CREATE_CALLABLE,  PHYS_CALL_BODY_CREATE,     "body_create");
PHYS_CALLABLE(PHYS_BODY_STATIC_CALLABLE,  PHYS_CALL_BODY_CREATE_STATIC, "body_create_static");
PHYS_CALLABLE(PHYS_BODY_DESTROY_CALLABLE, PHYS_CALL_BODY_DESTROY,    "body_destroy");
PHYS_CALLABLE(PHYS_BODY_SET_POS_CALLABLE, PHYS_CALL_BODY_SET_POS,    "body_set_pos");
PHYS_CALLABLE(PHYS_BODY_POS_X_CALLABLE,   PHYS_CALL_BODY_GET_POS_X,  "body_get_pos_x");
PHYS_CALLABLE(PHYS_BODY_POS_Y_CALLABLE,   PHYS_CALL_BODY_GET_POS_Y,  "body_get_pos_y");
PHYS_CALLABLE(PHYS_BODY_POS_Z_CALLABLE,   PHYS_CALL_BODY_GET_POS_Z,  "body_get_pos_z");
PHYS_CALLABLE(PHYS_BODY_FORCE_CALLABLE,   PHYS_CALL_BODY_APPLY_FORCE, "body_apply_force");
PHYS_CALLABLE(PHYS_BODY_IMPULSE_CALLABLE, PHYS_CALL_BODY_APPLY_IMPULSE, "body_apply_impulse");
PHYS_CALLABLE(PHYS_BODY_VEL_CALLABLE,     PHYS_CALL_BODY_SET_VELOCITY, "body_set_velocity");
PHYS_CALLABLE(PHYS_BODY_REST_CALLABLE,    PHYS_CALL_BODY_SET_RESTITUTION, "body_set_restitution");
PHYS_CALLABLE(PHYS_BODY_FRIC_CALLABLE,    PHYS_CALL_BODY_SET_FRICTION, "body_set_friction");
PHYS_CALLABLE(PHYS_SHAPE_SPHERE_CALLABLE, PHYS_CALL_SHAPE_SPHERE,    "SHAPE_SPHERE");
PHYS_CALLABLE(PHYS_SHAPE_BOX_CALLABLE,    PHYS_CALL_SHAPE_BOX,       "SHAPE_BOX");
PHYS_CALLABLE(PHYS_SHAPE_CAPSULE_CALLABLE,PHYS_CALL_SHAPE_CAPSULE,   "SHAPE_CAPSULE");
PHYS_CALLABLE(PHYS_SHAPE_PLANE_CALLABLE,  PHYS_CALL_SHAPE_PLANE,     "SHAPE_PLANE");
PHYS_CALLABLE(PHYS_RAYCAST_CALLABLE,      PHYS_CALL_RAYCAST,         "raycast");
PHYS_CALLABLE(PHYS_RAY_HIT_CALLABLE,      PHYS_CALL_RAY_HIT,         "ray_hit");
PHYS_CALLABLE(PHYS_RAY_DIST_CALLABLE,     PHYS_CALL_RAY_DISTANCE,    "ray_distance");
PHYS_CALLABLE(PHYS_PROJ_LAUNCH_CALLABLE,  PHYS_CALL_PROJECTILE_LAUNCH, "projectile_launch");
PHYS_CALLABLE(PHYS_CHAR_CREATE_CALLABLE,  PHYS_CALL_CHAR_CREATE,     "char_create");
PHYS_CALLABLE(PHYS_CHAR_MOVE_CALLABLE,    PHYS_CALL_CHAR_MOVE,       "char_move");
PHYS_CALLABLE(PHYS_CHAR_JUMP_CALLABLE,    PHYS_CALL_CHAR_JUMP,       "char_jump");
PHYS_CALLABLE(PHYS_CHAR_UPDATE_CALLABLE,  PHYS_CALL_CHAR_UPDATE,     "char_update");
PHYS_CALLABLE(PHYS_CHAR_GND_CALLABLE,     PHYS_CALL_CHAR_GROUNDED,   "char_grounded");

#undef PHYS_CALLABLE

static CalyndaRtExternCallable *ALL_PHYS_CALLABLES[] = {
    &PHYS_WORLD_INIT_CALLABLE, &PHYS_WORLD_STEP_CALLABLE, &PHYS_SET_GRAVITY_CALLABLE,
    &PHYS_BODY_CREATE_CALLABLE, &PHYS_BODY_STATIC_CALLABLE, &PHYS_BODY_DESTROY_CALLABLE,
    &PHYS_BODY_SET_POS_CALLABLE,
    &PHYS_BODY_POS_X_CALLABLE, &PHYS_BODY_POS_Y_CALLABLE, &PHYS_BODY_POS_Z_CALLABLE,
    &PHYS_BODY_FORCE_CALLABLE, &PHYS_BODY_IMPULSE_CALLABLE, &PHYS_BODY_VEL_CALLABLE,
    &PHYS_BODY_REST_CALLABLE, &PHYS_BODY_FRIC_CALLABLE,
    &PHYS_SHAPE_SPHERE_CALLABLE, &PHYS_SHAPE_BOX_CALLABLE,
    &PHYS_SHAPE_CAPSULE_CALLABLE, &PHYS_SHAPE_PLANE_CALLABLE,
    &PHYS_RAYCAST_CALLABLE, &PHYS_RAY_HIT_CALLABLE, &PHYS_RAY_DIST_CALLABLE,
    &PHYS_PROJ_LAUNCH_CALLABLE,
    &PHYS_CHAR_CREATE_CALLABLE, &PHYS_CHAR_MOVE_CALLABLE,
    &PHYS_CHAR_JUMP_CALLABLE, &PHYS_CHAR_UPDATE_CALLABLE, &PHYS_CHAR_GND_CALLABLE,
};
#define PHYS_CALLABLE_COUNT (sizeof(ALL_PHYS_CALLABLES) / sizeof(ALL_PHYS_CALLABLES[0]))

/* ------------------------------------------------------------------ */
/* Registration                                                         */
/* ------------------------------------------------------------------ */

void calynda_physics_register_objects(void) {
    calynda_rt_register_static_object(&__calynda_pkg_physics.header);
    for (size_t i = 0; i < PHYS_CALLABLE_COUNT; i++) {
        calynda_rt_register_static_object(&ALL_PHYS_CALLABLES[i]->header);
    }
}

/* ------------------------------------------------------------------ */
/* Dispatch                                                             */
/* ------------------------------------------------------------------ */

#ifdef CALYNDA_WII_BUILD
/* ----- Wii: static physics state ----- */
static PhysWorld   g_world;
static PhysCharController g_char;
static PhysRayResult g_last_ray;
static bool g_world_inited = false;
#endif

CalyndaRtWord calynda_physics_dispatch(const CalyndaRtExternCallable *callable,
                                        size_t argument_count,
                                        const CalyndaRtWord *arguments) {
    (void)argument_count;
    (void)arguments;

    switch ((int)callable->kind) {

    case PHYS_CALL_WORLD_INIT:
#ifdef CALYNDA_WII_BUILD
        phys_world_init(&g_world);
        g_world_inited = true;
#else
        fprintf(stderr, "[physics.world_init] stub\n");
#endif
        return 0;

    case PHYS_CALL_WORLD_STEP:
#ifdef CALYNDA_WII_BUILD
        if (g_world_inited) {
            float dt = (argument_count > 0) ? word_to_float(arguments[0]) : PHYS_FIXED_DT;
            phys_world_step(&g_world, dt);
        }
#else
        fprintf(stderr, "[physics.world_step] stub\n");
#endif
        return 0;

    case PHYS_CALL_SET_GRAVITY:
#ifdef CALYNDA_WII_BUILD
        if (g_world_inited && argument_count >= 3) {
            g_world.gravity = cm_vec3(word_to_float(arguments[0]),
                                       word_to_float(arguments[1]),
                                       word_to_float(arguments[2]));
        }
#else
        fprintf(stderr, "[physics.set_gravity] stub\n");
#endif
        return 0;

    /* ---- Body creation ---- */
    case PHYS_CALL_BODY_CREATE: {
#ifdef CALYNDA_WII_BUILD
        /* args: x, y, z, mass, shape_type [, shape params...] */
        if (g_world_inited && argument_count >= 5) {
            CmVec3 pos = cm_vec3(word_to_float(arguments[0]),
                                  word_to_float(arguments[1]),
                                  word_to_float(arguments[2]));
            float mass = word_to_float(arguments[3]);
            int shape = (int)arguments[4];
            PhysCollider col;
            memset(&col, 0, sizeof(col));
            switch (shape) {
                case 0: /* sphere */
                    col.type = PHYS_SHAPE_SPHERE;
                    col.sphere.radius = (argument_count > 5) ? word_to_float(arguments[5]) : 0.5f;
                    break;
                case 1: /* aabb */
                    col.type = PHYS_SHAPE_AABB;
                    col.aabb.half_extents = (argument_count > 7)
                        ? cm_vec3(word_to_float(arguments[5]),
                                   word_to_float(arguments[6]),
                                   word_to_float(arguments[7]))
                        : cm_vec3(0.5f, 0.5f, 0.5f);
                    break;
                case 2: /* capsule */
                    col.type = PHYS_SHAPE_CAPSULE;
                    col.capsule.radius = (argument_count > 5) ? word_to_float(arguments[5]) : 0.3f;
                    col.capsule.half_height = (argument_count > 6) ? word_to_float(arguments[6]) : 0.5f;
                    break;
                default: /* plane */
                    col.type = PHYS_SHAPE_PLANE;
                    col.plane.normal = cm_vec3(0, 1, 0);
                    col.plane.distance = 0;
                    break;
            }
            PhysBody *b = phys_body_create(&g_world, pos, mass, col);
            return (CalyndaRtWord)(uintptr_t)b;
        }
        return 0;
#else
        fprintf(stderr, "[physics.body_create] stub -> handle 1\n");
        return 1;
#endif
    }

    case PHYS_CALL_BODY_CREATE_STATIC: {
#ifdef CALYNDA_WII_BUILD
        /* args: x, y, z, shape_type [, shape params...] */
        if (g_world_inited && argument_count >= 4) {
            CmVec3 pos = cm_vec3(word_to_float(arguments[0]),
                                  word_to_float(arguments[1]),
                                  word_to_float(arguments[2]));
            int shape = (int)arguments[3];
            PhysCollider col;
            memset(&col, 0, sizeof(col));
            switch (shape) {
                case 0:
                    col.type = PHYS_SHAPE_SPHERE;
                    col.sphere.radius = (argument_count > 4) ? word_to_float(arguments[4]) : 0.5f;
                    break;
                case 1:
                    col.type = PHYS_SHAPE_AABB;
                    col.aabb.half_extents = (argument_count > 6)
                        ? cm_vec3(word_to_float(arguments[4]),
                                   word_to_float(arguments[5]),
                                   word_to_float(arguments[6]))
                        : cm_vec3(0.5f, 0.5f, 0.5f);
                    break;
                case 2:
                    col.type = PHYS_SHAPE_CAPSULE;
                    col.capsule.radius = (argument_count > 4) ? word_to_float(arguments[4]) : 0.3f;
                    col.capsule.half_height = (argument_count > 5) ? word_to_float(arguments[5]) : 0.5f;
                    break;
                default:
                    col.type = PHYS_SHAPE_PLANE;
                    col.plane.normal = cm_vec3(0, 1, 0);
                    col.plane.distance = 0;
                    break;
            }
            PhysBody *b = phys_body_create_static(&g_world, pos, col);
            return (CalyndaRtWord)(uintptr_t)b;
        }
        return 0;
#else
        fprintf(stderr, "[physics.body_create_static] stub -> handle 2\n");
        return 2;
#endif
    }

    case PHYS_CALL_BODY_DESTROY:
#ifdef CALYNDA_WII_BUILD
        if (g_world_inited && argument_count >= 1) {
            PhysBody *b = (PhysBody *)(uintptr_t)arguments[0];
            if (b) phys_body_destroy(&g_world, b);
        }
#else
        fprintf(stderr, "[physics.body_destroy] stub\n");
#endif
        return 0;

    case PHYS_CALL_BODY_SET_POS:
#ifdef CALYNDA_WII_BUILD
        if (argument_count >= 4) {
            PhysBody *b = (PhysBody *)(uintptr_t)arguments[0];
            if (b) b->position = cm_vec3(word_to_float(arguments[1]),
                                          word_to_float(arguments[2]),
                                          word_to_float(arguments[3]));
        }
#else
        fprintf(stderr, "[physics.body_set_pos] stub\n");
#endif
        return 0;

    case PHYS_CALL_BODY_GET_POS_X:
#ifdef CALYNDA_WII_BUILD
        if (argument_count >= 1) {
            PhysBody *b = (PhysBody *)(uintptr_t)arguments[0];
            if (b) return float_to_word(b->position.x);
        }
#endif
        return float_to_word(0);

    case PHYS_CALL_BODY_GET_POS_Y:
#ifdef CALYNDA_WII_BUILD
        if (argument_count >= 1) {
            PhysBody *b = (PhysBody *)(uintptr_t)arguments[0];
            if (b) return float_to_word(b->position.y);
        }
#endif
        return float_to_word(0);

    case PHYS_CALL_BODY_GET_POS_Z:
#ifdef CALYNDA_WII_BUILD
        if (argument_count >= 1) {
            PhysBody *b = (PhysBody *)(uintptr_t)arguments[0];
            if (b) return float_to_word(b->position.z);
        }
#endif
        return float_to_word(0);

    case PHYS_CALL_BODY_APPLY_FORCE:
#ifdef CALYNDA_WII_BUILD
        if (argument_count >= 4) {
            PhysBody *b = (PhysBody *)(uintptr_t)arguments[0];
            if (b) phys_body_apply_force(b, cm_vec3(word_to_float(arguments[1]),
                                                     word_to_float(arguments[2]),
                                                     word_to_float(arguments[3])));
        }
#else
        fprintf(stderr, "[physics.body_apply_force] stub\n");
#endif
        return 0;

    case PHYS_CALL_BODY_APPLY_IMPULSE:
#ifdef CALYNDA_WII_BUILD
        if (argument_count >= 4) {
            PhysBody *b = (PhysBody *)(uintptr_t)arguments[0];
            if (b) phys_body_apply_impulse(b, cm_vec3(word_to_float(arguments[1]),
                                                       word_to_float(arguments[2]),
                                                       word_to_float(arguments[3])));
        }
#else
        fprintf(stderr, "[physics.body_apply_impulse] stub\n");
#endif
        return 0;

    case PHYS_CALL_BODY_SET_VELOCITY:
#ifdef CALYNDA_WII_BUILD
        if (argument_count >= 4) {
            PhysBody *b = (PhysBody *)(uintptr_t)arguments[0];
            if (b) phys_body_set_velocity(b, cm_vec3(word_to_float(arguments[1]),
                                                      word_to_float(arguments[2]),
                                                      word_to_float(arguments[3])));
        }
#else
        fprintf(stderr, "[physics.body_set_velocity] stub\n");
#endif
        return 0;

    case PHYS_CALL_BODY_SET_RESTITUTION:
#ifdef CALYNDA_WII_BUILD
        if (argument_count >= 2) {
            PhysBody *b = (PhysBody *)(uintptr_t)arguments[0];
            if (b) b->restitution = word_to_float(arguments[1]);
        }
#else
        fprintf(stderr, "[physics.body_set_restitution] stub\n");
#endif
        return 0;

    case PHYS_CALL_BODY_SET_FRICTION:
#ifdef CALYNDA_WII_BUILD
        if (argument_count >= 2) {
            PhysBody *b = (PhysBody *)(uintptr_t)arguments[0];
            if (b) b->friction = word_to_float(arguments[1]);
        }
#else
        fprintf(stderr, "[physics.body_set_friction] stub\n");
#endif
        return 0;

    /* ---- Shape constants ---- */
    case PHYS_CALL_SHAPE_SPHERE:  return 0;
    case PHYS_CALL_SHAPE_BOX:     return 1;
    case PHYS_CALL_SHAPE_CAPSULE: return 2;
    case PHYS_CALL_SHAPE_PLANE:   return 3;

    /* ---- Raycast ---- */
    case PHYS_CALL_RAYCAST:
#ifdef CALYNDA_WII_BUILD
        /* args: ox, oy, oz, dx, dy, dz, max_dist */
        if (g_world_inited && argument_count >= 7) {
            CmRay ray;
            ray.origin = cm_vec3(word_to_float(arguments[0]),
                                  word_to_float(arguments[1]),
                                  word_to_float(arguments[2]));
            ray.direction = cm_vec3(word_to_float(arguments[3]),
                                     word_to_float(arguments[4]),
                                     word_to_float(arguments[5]));
            float max_dist = word_to_float(arguments[6]);
            g_last_ray = phys_raycast(&g_world, ray, max_dist, 0xFFFFFFFF);
        }
#else
        fprintf(stderr, "[physics.raycast] stub\n");
#endif
        return 0;

    case PHYS_CALL_RAY_HIT:
#ifdef CALYNDA_WII_BUILD
        return (CalyndaRtWord)g_last_ray.hit;
#else
        return 0;
#endif

    case PHYS_CALL_RAY_DISTANCE:
#ifdef CALYNDA_WII_BUILD
        return float_to_word(g_last_ray.distance);
#else
        return float_to_word(0);
#endif

    /* ---- Projectile ---- */
    case PHYS_CALL_PROJECTILE_LAUNCH: {
#ifdef CALYNDA_WII_BUILD
        /* args: px, py, pz, vx, vy, vz, mass, radius */
        if (g_world_inited && argument_count >= 8) {
            PhysProjectile proj;
            memset(&proj, 0, sizeof(proj));
            proj.position = cm_vec3(word_to_float(arguments[0]),
                                     word_to_float(arguments[1]),
                                     word_to_float(arguments[2]));
            proj.velocity = cm_vec3(word_to_float(arguments[3]),
                                     word_to_float(arguments[4]),
                                     word_to_float(arguments[5]));
            proj.mass   = word_to_float(arguments[6]);
            proj.radius = word_to_float(arguments[7]);
            proj.drag_coeff   = 0.47f;
            proj.magnus_coeff = 0.0f;
            proj.lift_coeff   = 0.0f;
            proj.spin = cm_vec3(0, 0, 0);
            PhysBody *b = phys_projectile_launch(&g_world, &proj);
            return (CalyndaRtWord)(uintptr_t)b;
        }
        return 0;
#else
        fprintf(stderr, "[physics.projectile_launch] stub -> handle 10\n");
        return 10;
#endif
    }

    /* ---- Character controller ---- */
    case PHYS_CALL_CHAR_CREATE:
#ifdef CALYNDA_WII_BUILD
        /* args: x, y, z, radius, height, mass */
        if (g_world_inited && argument_count >= 6) {
            CmVec3 pos = cm_vec3(word_to_float(arguments[0]),
                                  word_to_float(arguments[1]),
                                  word_to_float(arguments[2]));
            float radius = word_to_float(arguments[3]);
            float height = word_to_float(arguments[4]);
            float mass   = word_to_float(arguments[5]);
            g_char = phys_char_create(&g_world, pos, radius, height, mass);
            return (CalyndaRtWord)(uintptr_t)g_char.body;
        }
        return 0;
#else
        fprintf(stderr, "[physics.char_create] stub -> handle 20\n");
        return 20;
#endif

    case PHYS_CALL_CHAR_MOVE:
#ifdef CALYNDA_WII_BUILD
        if (argument_count >= 4) {
            CmVec3 dir = cm_vec3(word_to_float(arguments[0]),
                                  word_to_float(arguments[1]),
                                  word_to_float(arguments[2]));
            float dt = word_to_float(arguments[3]);
            phys_char_move(&g_char, dir, dt);
        }
#else
        fprintf(stderr, "[physics.char_move] stub\n");
#endif
        return 0;

    case PHYS_CALL_CHAR_JUMP:
#ifdef CALYNDA_WII_BUILD
        phys_char_jump(&g_char);
#else
        fprintf(stderr, "[physics.char_jump] stub\n");
#endif
        return 0;

    case PHYS_CALL_CHAR_UPDATE:
#ifdef CALYNDA_WII_BUILD
        if (argument_count >= 1) {
            float dt = word_to_float(arguments[0]);
            phys_char_update(&g_char, &g_world, dt);
        }
#else
        fprintf(stderr, "[physics.char_update] stub\n");
#endif
        return 0;

    case PHYS_CALL_CHAR_GROUNDED:
#ifdef CALYNDA_WII_BUILD
        return (CalyndaRtWord)g_char.grounded;
#else
        return 0;
#endif
    }

    fprintf(stderr, "runtime: unsupported physics callable kind %d\n",
            (int)callable->kind);
    return 0;
}

/* ------------------------------------------------------------------ */
/* Member Load                                                          */
/* ------------------------------------------------------------------ */

static CalyndaRtWord rt_make_obj(void *p) {
    return (CalyndaRtWord)(uintptr_t)p;
}

CalyndaRtWord calynda_physics_member_load(const char *member) {
    if (!member) return 0;

    if (strcmp(member, "world_init")      == 0) return rt_make_obj(&PHYS_WORLD_INIT_CALLABLE);
    if (strcmp(member, "world_step")      == 0) return rt_make_obj(&PHYS_WORLD_STEP_CALLABLE);
    if (strcmp(member, "set_gravity")     == 0) return rt_make_obj(&PHYS_SET_GRAVITY_CALLABLE);
    if (strcmp(member, "body_create")     == 0) return rt_make_obj(&PHYS_BODY_CREATE_CALLABLE);
    if (strcmp(member, "body_create_static") == 0) return rt_make_obj(&PHYS_BODY_STATIC_CALLABLE);
    if (strcmp(member, "body_destroy")    == 0) return rt_make_obj(&PHYS_BODY_DESTROY_CALLABLE);
    if (strcmp(member, "body_set_pos")    == 0) return rt_make_obj(&PHYS_BODY_SET_POS_CALLABLE);
    if (strcmp(member, "body_get_pos_x")  == 0) return rt_make_obj(&PHYS_BODY_POS_X_CALLABLE);
    if (strcmp(member, "body_get_pos_y")  == 0) return rt_make_obj(&PHYS_BODY_POS_Y_CALLABLE);
    if (strcmp(member, "body_get_pos_z")  == 0) return rt_make_obj(&PHYS_BODY_POS_Z_CALLABLE);
    if (strcmp(member, "body_apply_force") == 0) return rt_make_obj(&PHYS_BODY_FORCE_CALLABLE);
    if (strcmp(member, "body_apply_impulse") == 0) return rt_make_obj(&PHYS_BODY_IMPULSE_CALLABLE);
    if (strcmp(member, "body_set_velocity") == 0) return rt_make_obj(&PHYS_BODY_VEL_CALLABLE);
    if (strcmp(member, "body_set_restitution") == 0) return rt_make_obj(&PHYS_BODY_REST_CALLABLE);
    if (strcmp(member, "body_set_friction") == 0) return rt_make_obj(&PHYS_BODY_FRIC_CALLABLE);
    if (strcmp(member, "SHAPE_SPHERE")   == 0) return rt_make_obj(&PHYS_SHAPE_SPHERE_CALLABLE);
    if (strcmp(member, "SHAPE_BOX")      == 0) return rt_make_obj(&PHYS_SHAPE_BOX_CALLABLE);
    if (strcmp(member, "SHAPE_CAPSULE")  == 0) return rt_make_obj(&PHYS_SHAPE_CAPSULE_CALLABLE);
    if (strcmp(member, "SHAPE_PLANE")    == 0) return rt_make_obj(&PHYS_SHAPE_PLANE_CALLABLE);
    if (strcmp(member, "raycast")         == 0) return rt_make_obj(&PHYS_RAYCAST_CALLABLE);
    if (strcmp(member, "ray_hit")         == 0) return rt_make_obj(&PHYS_RAY_HIT_CALLABLE);
    if (strcmp(member, "ray_distance")    == 0) return rt_make_obj(&PHYS_RAY_DIST_CALLABLE);
    if (strcmp(member, "projectile_launch") == 0) return rt_make_obj(&PHYS_PROJ_LAUNCH_CALLABLE);
    if (strcmp(member, "char_create")     == 0) return rt_make_obj(&PHYS_CHAR_CREATE_CALLABLE);
    if (strcmp(member, "char_move")       == 0) return rt_make_obj(&PHYS_CHAR_MOVE_CALLABLE);
    if (strcmp(member, "char_jump")       == 0) return rt_make_obj(&PHYS_CHAR_JUMP_CALLABLE);
    if (strcmp(member, "char_update")     == 0) return rt_make_obj(&PHYS_CHAR_UPDATE_CALLABLE);
    if (strcmp(member, "char_grounded")   == 0) return rt_make_obj(&PHYS_CHAR_GND_CALLABLE);

    fprintf(stderr, "runtime: physics has no member '%s'\n", member);
    return 0;
}
