/*
 * test_physics.c — Unit tests for calynda_physics.
 */

#include "calynda_physics.h"
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

#define ASSERT_VEC3_NEAR(v, ex, ey, ez, eps) do { \
    ASSERT_NEAR((v).x, ex, eps); \
    ASSERT_NEAR((v).y, ey, eps); \
    ASSERT_NEAR((v).z, ez, eps); \
} while(0)

#define RUN_TEST(fn) do { \
    int _before = tests_failed; \
    fn(); \
    printf("  %s: %s\n", (tests_failed == _before ? "PASS" : "FAIL"), #fn); \
} while(0)

/* ================================================================== */
/* World + Body tests                                                   */
/* ================================================================== */

static void test_world_init(void) {
    PhysWorld world;
    phys_world_init(&world);
    ASSERT_TRUE(world.body_count == 0);
    ASSERT_TRUE(world.constraint_count == 0);
    ASSERT_NEAR(world.gravity.y, -9.81f, 0.01f);
    ASSERT_TRUE(world.solver_iterations == 4);
}

static void test_body_create(void) {
    PhysWorld world;
    phys_world_init(&world);

    PhysCollider col;
    col.type = PHYS_SHAPE_SPHERE;
    col.sphere.radius = 1.0f;

    PhysBody *b = phys_body_create(&world, cm_vec3(0, 5, 0), 2.0f, col);
    ASSERT_TRUE(b != NULL);
    ASSERT_TRUE(world.body_count == 1);
    ASSERT_NEAR(b->mass, 2.0f, 0.001f);
    ASSERT_NEAR(b->inv_mass, 0.5f, 0.001f);
    ASSERT_TRUE(b->active);
    ASSERT_TRUE(!b->is_static);
    ASSERT_NEAR(b->position.y, 5.0f, 0.001f);
}

static void test_body_create_static(void) {
    PhysWorld world;
    phys_world_init(&world);

    PhysCollider col;
    col.type = PHYS_SHAPE_PLANE;
    col.plane.normal = cm_vec3(0, 1, 0);
    col.plane.distance = 0;

    PhysBody *b = phys_body_create_static(&world, cm_vec3(0, 0, 0), col);
    ASSERT_TRUE(b != NULL);
    ASSERT_TRUE(b->is_static);
    ASSERT_NEAR(b->inv_mass, 0, 0.001f);
}

static void test_body_destroy(void) {
    PhysWorld world;
    phys_world_init(&world);

    PhysCollider col;
    col.type = PHYS_SHAPE_SPHERE;
    col.sphere.radius = 1.0f;

    PhysBody *b1 = phys_body_create(&world, cm_vec3(0, 0, 0), 1.0f, col);
    PhysBody *b2 = phys_body_create(&world, cm_vec3(5, 0, 0), 1.0f, col);
    ASSERT_TRUE(world.body_count == 2);

    phys_body_destroy(&world, b1);
    ASSERT_TRUE(world.body_count == 1);
    /* The remaining body should still be valid */
    ASSERT_TRUE(world.bodies[0].active);
    (void)b2;
}

static void test_body_apply_force(void) {
    PhysWorld world;
    phys_world_init(&world);

    PhysCollider col;
    col.type = PHYS_SHAPE_SPHERE;
    col.sphere.radius = 1.0f;

    PhysBody *b = phys_body_create(&world, cm_vec3(0, 0, 0), 1.0f, col);
    phys_body_apply_force(b, cm_vec3(10, 0, 0));
    ASSERT_NEAR(b->force.x, 10.0f, 0.001f);

    phys_body_apply_force(b, cm_vec3(5, 0, 0));
    ASSERT_NEAR(b->force.x, 15.0f, 0.001f);
}

static void test_body_apply_impulse(void) {
    PhysWorld world;
    phys_world_init(&world);

    PhysCollider col;
    col.type = PHYS_SHAPE_SPHERE;
    col.sphere.radius = 1.0f;

    PhysBody *b = phys_body_create(&world, cm_vec3(0, 0, 0), 2.0f, col);
    phys_body_apply_impulse(b, cm_vec3(4, 0, 0));
    /* impulse / mass = delta_v: 4 / 2 = 2 */
    ASSERT_NEAR(b->linear_velocity.x, 2.0f, 0.001f);
}

static void test_body_set_velocity(void) {
    PhysWorld world;
    phys_world_init(&world);

    PhysCollider col;
    col.type = PHYS_SHAPE_SPHERE;
    col.sphere.radius = 1.0f;

    PhysBody *b = phys_body_create(&world, cm_vec3(0, 0, 0), 1.0f, col);
    phys_body_set_velocity(b, cm_vec3(3, 4, 5));
    ASSERT_VEC3_NEAR(b->linear_velocity, 3, 4, 5, 0.001f);
}

/* ================================================================== */
/* Integration tests                                                    */
/* ================================================================== */

static void test_gravity_fall(void) {
    PhysWorld world;
    phys_world_init(&world);

    PhysCollider col;
    col.type = PHYS_SHAPE_SPHERE;
    col.sphere.radius = 0.5f;

    PhysBody *b = phys_body_create(&world, cm_vec3(0, 10, 0), 1.0f, col);

    /* Step for 1 second (60 steps) */
    for (int i = 0; i < 60; i++) {
        phys_world_step(&world, PHYS_FIXED_DT);
    }

    /* After 1s of freefall: y ~= 10 - 0.5*g*t^2 = 10 - 4.905 ~= 5.1
       With damping it will be slightly different */
    ASSERT_TRUE(b->position.y < 10.0f);
    ASSERT_TRUE(b->position.y > 0.0f);
    /* Velocity should be roughly -9.81 (with some damping) */
    ASSERT_TRUE(b->linear_velocity.y < -5.0f);
}

static void test_static_body_no_move(void) {
    PhysWorld world;
    phys_world_init(&world);

    PhysCollider col;
    col.type = PHYS_SHAPE_AABB;
    col.aabb.half_extents = cm_vec3(10, 0.5f, 10);

    PhysBody *b = phys_body_create_static(&world, cm_vec3(0, 0, 0), col);

    for (int i = 0; i < 60; i++) {
        phys_world_step(&world, PHYS_FIXED_DT);
    }

    ASSERT_NEAR(b->position.x, 0, 0.001f);
    ASSERT_NEAR(b->position.y, 0, 0.001f);
    ASSERT_NEAR(b->position.z, 0, 0.001f);
}

/* ================================================================== */
/* Collision tests                                                      */
/* ================================================================== */

static void test_sphere_vs_plane_collision(void) {
    PhysWorld world;
    phys_world_init(&world);

    /* Ground plane */
    PhysCollider plane_col;
    plane_col.type = PHYS_SHAPE_PLANE;
    plane_col.plane.normal = cm_vec3(0, 1, 0);
    plane_col.plane.distance = 0;
    phys_body_create_static(&world, cm_vec3(0, 0, 0), plane_col);

    /* Sphere just above ground */
    PhysCollider sphere_col;
    sphere_col.type = PHYS_SHAPE_SPHERE;
    sphere_col.sphere.radius = 1.0f;
    PhysBody *ball = phys_body_create(&world, cm_vec3(0, 1.5f, 0), 1.0f, sphere_col);

    /* Let it fall */
    for (int i = 0; i < 120; i++) {
        phys_world_step(&world, PHYS_FIXED_DT);
    }

    /* Ball should settle near y=1 (radius above plane) */
    ASSERT_TRUE(ball->position.y >= 0.5f);
    ASSERT_TRUE(ball->position.y < 2.5f);
}

static void test_sphere_vs_sphere_collision(void) {
    PhysWorld world;
    phys_world_init(&world);
    world.gravity = cm_vec3(0, 0, 0); /* no gravity for this test */

    PhysCollider col;
    col.type = PHYS_SHAPE_SPHERE;
    col.sphere.radius = 1.0f;

    PhysBody *a = phys_body_create(&world, cm_vec3(-1, 0, 0), 1.0f, col);
    PhysBody *b = phys_body_create(&world, cm_vec3(1, 0, 0), 1.0f, col);

    /* Spheres overlap (distance=2, combined radii=2, barely touching) */
    /* Push them closer */
    a->position = cm_vec3(-0.5f, 0, 0);
    b->position = cm_vec3(0.5f, 0, 0);

    /* Give them velocities toward each other */
    a->linear_velocity = cm_vec3(5, 0, 0);
    b->linear_velocity = cm_vec3(-5, 0, 0);

    phys_world_step(&world, PHYS_FIXED_DT);

    /* After collision, they should bounce apart */
    /* A should be moving left, B should be moving right (or at least separated) */
    ASSERT_TRUE(a->linear_velocity.x < 3.0f); /* lost some velocity */
    ASSERT_TRUE(b->linear_velocity.x > -3.0f);
}

static void test_aabb_vs_aabb_collision(void) {
    PhysWorld world;
    phys_world_init(&world);
    world.gravity = cm_vec3(0, 0, 0);

    PhysCollider col;
    col.type = PHYS_SHAPE_AABB;
    col.aabb.half_extents = cm_vec3(1, 1, 1);

    /* Overlapping boxes */
    PhysBody *a = phys_body_create(&world, cm_vec3(0, 0, 0), 1.0f, col);
    PhysBody *b = phys_body_create(&world, cm_vec3(1.5f, 0, 0), 1.0f, col);

    phys_world_step(&world, PHYS_FIXED_DT);

    /* Boxes should be pushed apart */
    ASSERT_TRUE(a->position.x < b->position.x);
}

static void test_sphere_vs_aabb_collision(void) {
    PhysWorld world;
    phys_world_init(&world);
    world.gravity = cm_vec3(0, 0, 0);

    PhysCollider sphere_col;
    sphere_col.type = PHYS_SHAPE_SPHERE;
    sphere_col.sphere.radius = 1.0f;

    PhysCollider box_col;
    box_col.type = PHYS_SHAPE_AABB;
    box_col.aabb.half_extents = cm_vec3(1, 1, 1);

    PhysBody *sphere = phys_body_create(&world, cm_vec3(0, 0, 0), 1.0f, sphere_col);
    PhysBody *box = phys_body_create_static(&world, cm_vec3(1.5f, 0, 0), box_col);

    sphere->linear_velocity = cm_vec3(5, 0, 0);

    for (int i = 0; i < 5; i++) {
        phys_world_step(&world, PHYS_FIXED_DT);
    }

    /* Sphere should have bounced back */
    ASSERT_TRUE(sphere->linear_velocity.x < 5.0f);
    (void)box;
}

/* ================================================================== */
/* Constraint tests                                                     */
/* ================================================================== */

static void test_pin_constraint(void) {
    PhysWorld world;
    phys_world_init(&world);
    world.gravity = cm_vec3(0, 0, 0);

    PhysCollider col;
    col.type = PHYS_SHAPE_SPHERE;
    col.sphere.radius = 0.5f;

    PhysBody *b = phys_body_create(&world, cm_vec3(0, 5, 0), 1.0f, col);
    PhysConstraint *c = phys_constraint_pin(&world, b, cm_vec3(0, 0, 0), cm_vec3(0, 5, 0));
    ASSERT_TRUE(c != NULL);
    ASSERT_TRUE(c->active);

    /* Apply force trying to move body away */
    phys_body_apply_impulse(b, cm_vec3(10, 0, 0));

    for (int i = 0; i < 30; i++) {
        phys_world_step(&world, PHYS_FIXED_DT);
    }

    /* Body should stay near the pin position */
    float dist = cm_vec3_length(cm_vec3_sub(b->position, cm_vec3(0, 5, 0)));
    ASSERT_TRUE(dist < 5.0f); /* should be constrained somewhat */
}

static void test_distance_constraint(void) {
    PhysWorld world;
    phys_world_init(&world);
    world.gravity = cm_vec3(0, 0, 0);

    PhysCollider col;
    col.type = PHYS_SHAPE_SPHERE;
    col.sphere.radius = 0.5f;

    PhysBody *a = phys_body_create(&world, cm_vec3(0, 0, 0), 1.0f, col);
    PhysBody *b = phys_body_create(&world, cm_vec3(3, 0, 0), 1.0f, col);

    PhysConstraint *c = phys_constraint_distance(&world, a, cm_vec3(0,0,0), b, cm_vec3(0,0,0), 3.0f);
    ASSERT_TRUE(c != NULL);
    ASSERT_NEAR(c->distance, 3.0f, 0.001f);
}

/* ================================================================== */
/* Projectile tests                                                     */
/* ================================================================== */

static void test_projectile_launch(void) {
    PhysWorld world;
    phys_world_init(&world);

    PhysProjectile proj;
    memset(&proj, 0, sizeof(proj));
    proj.position = cm_vec3(0, 1, 0);
    proj.velocity = cm_vec3(10, 10, 0);
    proj.mass = 0.5f;
    proj.radius = 0.1f;
    proj.drag_coeff = 0.01f;
    proj.magnus_coeff = 0;

    PhysBody *b = phys_projectile_launch(&world, &proj);
    ASSERT_TRUE(b != NULL);
    ASSERT_NEAR(b->linear_velocity.x, 10.0f, 0.001f);
    ASSERT_NEAR(b->linear_velocity.y, 10.0f, 0.001f);
    ASSERT_NEAR(b->mass, 0.5f, 0.001f);
}

static void test_projectile_drag(void) {
    PhysWorld world;
    phys_world_init(&world);
    world.gravity = cm_vec3(0, 0, 0); /* no gravity */

    PhysProjectile proj;
    memset(&proj, 0, sizeof(proj));
    proj.position = cm_vec3(0, 0, 0);
    proj.velocity = cm_vec3(20, 0, 0);
    proj.mass = 1.0f;
    proj.radius = 0.5f;
    proj.drag_coeff = 0.1f;

    PhysBody *b = phys_projectile_launch(&world, &proj);

    /* Apply drag force for several steps */
    for (int i = 0; i < 60; i++) {
        phys_projectile_apply_forces(b, &proj);
        phys_world_step(&world, PHYS_FIXED_DT);
    }

    /* Velocity should be reduced by drag */
    ASSERT_TRUE(b->linear_velocity.x < 20.0f);
    ASSERT_TRUE(b->linear_velocity.x > 0); /* still moving forward */
}

static void test_projectile_magnus(void) {
    PhysWorld world;
    phys_world_init(&world);
    world.gravity = cm_vec3(0, 0, 0);

    PhysProjectile proj;
    memset(&proj, 0, sizeof(proj));
    proj.position = cm_vec3(0, 0, 0);
    proj.velocity = cm_vec3(10, 0, 0);
    proj.spin = cm_vec3(0, 10, 0);  /* topspin around Y */
    proj.mass = 1.0f;
    proj.radius = 0.1f;
    proj.drag_coeff = 0;
    proj.magnus_coeff = 0.5f;

    PhysBody *b = phys_projectile_launch(&world, &proj);

    for (int i = 0; i < 30; i++) {
        phys_projectile_apply_forces(b, &proj);
        phys_world_step(&world, PHYS_FIXED_DT);
    }

    /* Magnus force (spin x vel) with Y spin and X velocity should produce Z force */
    ASSERT_TRUE(fabsf(b->position.z) > 0.01f);
}

/* ================================================================== */
/* Buoyancy tests                                                       */
/* ================================================================== */

static void test_buoyancy_submerged(void) {
    PhysWorld world;
    phys_world_init(&world);
    world.gravity = cm_vec3(0, -9.81f, 0);

    PhysCollider col;
    col.type = PHYS_SHAPE_SPHERE;
    col.sphere.radius = 1.0f;

    PhysBody *b = phys_body_create(&world, cm_vec3(0, -2, 0), 1.0f, col);

    /* Apply buoyancy (water at y=0) */
    phys_apply_buoyancy(b, 0.0f, 1.0f);

    /* Force should have positive Y component (upward) */
    ASSERT_TRUE(b->force.y > 0);
}

static void test_buoyancy_above_water(void) {
    PhysWorld world;
    phys_world_init(&world);

    PhysCollider col;
    col.type = PHYS_SHAPE_SPHERE;
    col.sphere.radius = 1.0f;

    PhysBody *b = phys_body_create(&world, cm_vec3(0, 5, 0), 1.0f, col);
    phys_apply_buoyancy(b, 0.0f, 1.0f);

    /* No buoyancy when above water */
    ASSERT_NEAR(b->force.y, 0, 0.001f);
}

/* ================================================================== */
/* Character controller tests                                           */
/* ================================================================== */

static void test_char_create(void) {
    PhysWorld world;
    phys_world_init(&world);

    PhysCharController ctrl = phys_char_create(&world, cm_vec3(0, 2, 0), 0.4f, 1.8f, 75.0f);
    ASSERT_TRUE(ctrl.body != NULL);
    ASSERT_NEAR(ctrl.move_speed, 5.0f, 0.001f);
    ASSERT_NEAR(ctrl.jump_force, 6.0f, 0.001f);
    ASSERT_TRUE(!ctrl.grounded);
}

static void test_char_move(void) {
    PhysWorld world;
    phys_world_init(&world);
    world.gravity = cm_vec3(0, 0, 0);

    PhysCharController ctrl = phys_char_create(&world, cm_vec3(0, 0, 0), 0.4f, 1.8f, 75.0f);
    phys_char_move(&ctrl, cm_vec3(1, 0, 0), PHYS_FIXED_DT);

    ASSERT_NEAR(ctrl.body->linear_velocity.x, 5.0f, 0.001f);
}

/* ================================================================== */
/* Raycast tests                                                        */
/* ================================================================== */

static void test_raycast_hit_sphere(void) {
    PhysWorld world;
    phys_world_init(&world);

    PhysCollider col;
    col.type = PHYS_SHAPE_SPHERE;
    col.sphere.radius = 1.0f;

    phys_body_create(&world, cm_vec3(5, 0, 0), 1.0f, col);

    CmRay ray;
    ray.origin = cm_vec3(0, 0, 0);
    ray.direction = cm_vec3(1, 0, 0);

    PhysRayResult result = phys_raycast(&world, ray, 100.0f, 0xFFFFFFFF);
    ASSERT_TRUE(result.hit);
    ASSERT_TRUE(result.body != NULL);
    ASSERT_NEAR(result.distance, 4.0f, 0.1f); /* 5 - radius */
}

static void test_raycast_miss(void) {
    PhysWorld world;
    phys_world_init(&world);

    PhysCollider col;
    col.type = PHYS_SHAPE_SPHERE;
    col.sphere.radius = 1.0f;

    phys_body_create(&world, cm_vec3(5, 5, 0), 1.0f, col);

    CmRay ray;
    ray.origin = cm_vec3(0, 0, 0);
    ray.direction = cm_vec3(1, 0, 0); /* shoots along X, sphere is at (5,5,0) */

    PhysRayResult result = phys_raycast(&world, ray, 100.0f, 0xFFFFFFFF);
    ASSERT_TRUE(!result.hit);
}

static void test_raycast_hit_plane(void) {
    PhysWorld world;
    phys_world_init(&world);

    PhysCollider col;
    col.type = PHYS_SHAPE_PLANE;
    col.plane.normal = cm_vec3(0, 1, 0);
    col.plane.distance = 0;

    phys_body_create_static(&world, cm_vec3(0, 0, 0), col);

    CmRay ray;
    ray.origin = cm_vec3(0, 5, 0);
    ray.direction = cm_vec3(0, -1, 0);

    PhysRayResult result = phys_raycast(&world, ray, 100.0f, 0xFFFFFFFF);
    ASSERT_TRUE(result.hit);
    ASSERT_NEAR(result.distance, 5.0f, 0.1f);
}

static void test_raycast_layer_filter(void) {
    PhysWorld world;
    phys_world_init(&world);

    PhysCollider col;
    col.type = PHYS_SHAPE_SPHERE;
    col.sphere.radius = 1.0f;

    PhysBody *b = phys_body_create(&world, cm_vec3(5, 0, 0), 1.0f, col);
    b->collision_layer = 2; /* layer 2 */

    CmRay ray;
    ray.origin = cm_vec3(0, 0, 0);
    ray.direction = cm_vec3(1, 0, 0);

    /* Test with layer 1 mask — should miss */
    PhysRayResult result = phys_raycast(&world, ray, 100.0f, 1);
    ASSERT_TRUE(!result.hit);

    /* Test with layer 2 mask — should hit */
    result = phys_raycast(&world, ray, 100.0f, 2);
    ASSERT_TRUE(result.hit);
}

/* ================================================================== */
/* Collision mask tests                                                 */
/* ================================================================== */

static void test_collision_mask_filter(void) {
    PhysWorld world;
    phys_world_init(&world);
    world.gravity = cm_vec3(0, 0, 0);

    PhysCollider col;
    col.type = PHYS_SHAPE_SPHERE;
    col.sphere.radius = 1.0f;

    /* Two overlapping spheres on different layers */
    PhysBody *a = phys_body_create(&world, cm_vec3(0, 0, 0), 1.0f, col);
    PhysBody *b = phys_body_create(&world, cm_vec3(0.5f, 0, 0), 1.0f, col);

    a->collision_layer = 1;
    a->collision_mask = 1;
    b->collision_layer = 2;
    b->collision_mask = 2;

    CmVec3 a_pos_before = a->position;

    phys_world_step(&world, PHYS_FIXED_DT);

    /* Bodies should NOT collide since masks don't match layers */
    /* Position should only change from damping, not from collision */
    ASSERT_NEAR(a->position.x, a_pos_before.x, 0.1f);
}

/* ================================================================== */
/* Sensor test                                                          */
/* ================================================================== */

static int sensor_callback_count = 0;
static void sensor_callback(PhysBody *a, PhysBody *b, const PhysContact *c, void *ud) {
    (void)a; (void)b; (void)c; (void)ud;
    sensor_callback_count++;
}

static void test_sensor_no_response(void) {
    PhysWorld world;
    phys_world_init(&world);
    world.gravity = cm_vec3(0, 0, 0);

    sensor_callback_count = 0;
    phys_world_set_callback(&world, sensor_callback, NULL);

    PhysCollider col;
    col.type = PHYS_SHAPE_SPHERE;
    col.sphere.radius = 1.0f;

    PhysBody *sensor = phys_body_create(&world, cm_vec3(0, 0, 0), 1.0f, col);
    sensor->is_sensor = true;

    PhysBody *b = phys_body_create(&world, cm_vec3(0.5f, 0, 0), 1.0f, col);

    phys_world_step(&world, PHYS_FIXED_DT);

    /* Callback should have fired */
    ASSERT_TRUE(sensor_callback_count > 0);

    /* Sensor should not have been pushed (impulse solver skips sensors) */
    /* Velocity should remain ~0 (no collision response) */
    ASSERT_NEAR(sensor->linear_velocity.x, 0, 0.5f);
    (void)b;
}

/* ================================================================== */
/* Capsule collision tests                                              */
/* ================================================================== */

static void test_capsule_vs_plane(void) {
    PhysWorld world;
    phys_world_init(&world);

    /* Ground plane */
    PhysCollider plane_col;
    plane_col.type = PHYS_SHAPE_PLANE;
    plane_col.plane.normal = cm_vec3(0, 1, 0);
    plane_col.plane.distance = 0;
    phys_body_create_static(&world, cm_vec3(0, 0, 0), plane_col);

    /* Capsule above ground */
    PhysCollider cap_col;
    cap_col.type = PHYS_SHAPE_CAPSULE;
    cap_col.capsule.radius = 0.5f;
    cap_col.capsule.half_height = 1.0f;

    PhysBody *cap = phys_body_create(&world, cm_vec3(0, 3, 0), 1.0f, cap_col);

    for (int i = 0; i < 120; i++) {
        phys_world_step(&world, PHYS_FIXED_DT);
    }

    /* Capsule should settle above ground */
    ASSERT_TRUE(cap->position.y >= 0.5f);
    ASSERT_TRUE(cap->position.y < 5.0f);
}

static void test_capsule_vs_capsule(void) {
    PhysWorld world;
    phys_world_init(&world);
    world.gravity = cm_vec3(0, 0, 0);

    PhysCollider cap_col;
    cap_col.type = PHYS_SHAPE_CAPSULE;
    cap_col.capsule.radius = 0.5f;
    cap_col.capsule.half_height = 1.0f;

    PhysBody *a = phys_body_create(&world, cm_vec3(0, 0, 0), 1.0f, cap_col);
    PhysBody *b = phys_body_create(&world, cm_vec3(0.5f, 0, 0), 1.0f, cap_col);

    a->linear_velocity = cm_vec3(2, 0, 0);
    b->linear_velocity = cm_vec3(-2, 0, 0);

    phys_world_step(&world, PHYS_FIXED_DT);

    /* Should separate */
    ASSERT_TRUE(a->position.x < b->position.x);
}

/* ================================================================== */
/* Multi-step simulation test                                           */
/* ================================================================== */

static void test_ball_bounce_on_ground(void) {
    PhysWorld world;
    phys_world_init(&world);

    /* Ground plane */
    PhysCollider plane_col;
    plane_col.type = PHYS_SHAPE_PLANE;
    plane_col.plane.normal = cm_vec3(0, 1, 0);
    plane_col.plane.distance = 0;
    phys_body_create_static(&world, cm_vec3(0, 0, 0), plane_col);

    /* Ball dropped from height */
    PhysCollider sphere_col;
    sphere_col.type = PHYS_SHAPE_SPHERE;
    sphere_col.sphere.radius = 0.5f;

    PhysBody *ball = phys_body_create(&world, cm_vec3(0, 5, 0), 1.0f, sphere_col);
    ball->restitution = 0.7f;

    float max_height = 5.0f;

    /* Simulate for 3 seconds */
    for (int i = 0; i < 180; i++) {
        phys_world_step(&world, PHYS_FIXED_DT);
        if (ball->position.y > max_height) max_height = ball->position.y;
    }

    /* Ball should have lost energy */
    ASSERT_TRUE(ball->position.y < 5.0f);
    /* Ball should not have gone below ground (much) */
    ASSERT_TRUE(ball->position.y >= -0.5f);
}

/* ================================================================== */
/* Collision callback test                                              */
/* ================================================================== */

static int callback_call_count = 0;
static void test_collision_cb(PhysBody *a, PhysBody *b, const PhysContact *c, void *ud) {
    (void)a; (void)b; (void)c; (void)ud;
    callback_call_count++;
}

static void test_collision_callback(void) {
    PhysWorld world;
    phys_world_init(&world);
    world.gravity = cm_vec3(0, 0, 0);

    callback_call_count = 0;
    phys_world_set_callback(&world, test_collision_cb, NULL);

    PhysCollider col;
    col.type = PHYS_SHAPE_SPHERE;
    col.sphere.radius = 1.0f;

    /* Two overlapping spheres */
    phys_body_create(&world, cm_vec3(0, 0, 0), 1.0f, col);
    phys_body_create(&world, cm_vec3(1, 0, 0), 1.0f, col);

    phys_world_step(&world, PHYS_FIXED_DT);

    ASSERT_TRUE(callback_call_count > 0);
}

/* ================================================================== */
/* Main                                                                 */
/* ================================================================== */

int main(void) {
    printf("=== calynda_physics tests ===\n");

    printf("\n--- World & Body ---\n");
    RUN_TEST(test_world_init);
    RUN_TEST(test_body_create);
    RUN_TEST(test_body_create_static);
    RUN_TEST(test_body_destroy);
    RUN_TEST(test_body_apply_force);
    RUN_TEST(test_body_apply_impulse);
    RUN_TEST(test_body_set_velocity);

    printf("\n--- Integration ---\n");
    RUN_TEST(test_gravity_fall);
    RUN_TEST(test_static_body_no_move);

    printf("\n--- Collision ---\n");
    RUN_TEST(test_sphere_vs_plane_collision);
    RUN_TEST(test_sphere_vs_sphere_collision);
    RUN_TEST(test_aabb_vs_aabb_collision);
    RUN_TEST(test_sphere_vs_aabb_collision);
    RUN_TEST(test_collision_mask_filter);
    RUN_TEST(test_sensor_no_response);
    RUN_TEST(test_capsule_vs_plane);
    RUN_TEST(test_capsule_vs_capsule);
    RUN_TEST(test_ball_bounce_on_ground);
    RUN_TEST(test_collision_callback);

    printf("\n--- Constraints ---\n");
    RUN_TEST(test_pin_constraint);
    RUN_TEST(test_distance_constraint);

    printf("\n--- Projectile ---\n");
    RUN_TEST(test_projectile_launch);
    RUN_TEST(test_projectile_drag);
    RUN_TEST(test_projectile_magnus);

    printf("\n--- Buoyancy ---\n");
    RUN_TEST(test_buoyancy_submerged);
    RUN_TEST(test_buoyancy_above_water);

    printf("\n--- Character Controller ---\n");
    RUN_TEST(test_char_create);
    RUN_TEST(test_char_move);

    printf("\n--- Raycast ---\n");
    RUN_TEST(test_raycast_hit_sphere);
    RUN_TEST(test_raycast_miss);
    RUN_TEST(test_raycast_hit_plane);
    RUN_TEST(test_raycast_layer_filter);

    printf("\n===========================\n");
    printf("Total: %d  Passed: %d  Failed: %d\n", tests_run, tests_passed, tests_failed);
    return tests_failed > 0 ? 1 : 0;
}
