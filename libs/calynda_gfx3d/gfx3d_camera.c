/*
 * gfx3d_camera.c — Camera system implementation.
 */

#include "gfx3d_camera.h"

#include <math.h>

#ifdef CALYNDA_WII_BUILD
#include <gccore.h>
#endif

/* Cached view matrix from the last camera_apply call */
static CmMat4 g_active_view = {{1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1}};

CmMat4 gfx3d_camera_active_view(void) {
    return g_active_view;
}

Gfx3dCamera gfx3d_camera_default(float aspect) {
    Gfx3dCamera cam;
    cam.position = cm_vec3(0, 5, 10);
    cam.target   = cm_vec3(0, 0, 0);
    cam.up       = cm_vec3(0, 1, 0);
    cam.fov_y    = 60.0f;
    cam.near_z   = 0.1f;
    cam.far_z    = 1000.0f;
    cam.aspect   = aspect;
    cam.mode     = GFX3D_CAM_FREE;

    cam.orbit_distance = 10.0f;
    cam.orbit_yaw      = 0.0f;
    cam.orbit_pitch    = 0.3f;   /* ~17° above horizon */

    cam.follow_offset = cm_vec3(0, 3, 8);
    cam.follow_speed  = 0.1f;

    return cam;
}

CmMat4 gfx3d_camera_view(const Gfx3dCamera *cam) {
    return cm_lookat(cam->position, cam->target, cam->up);
}

CmMat4 gfx3d_camera_projection(const Gfx3dCamera *cam) {
    return cm_perspective(cam->fov_y, cam->aspect, cam->near_z, cam->far_z);
}

CmMat4 gfx3d_camera_view_projection(const Gfx3dCamera *cam) {
    return cm_mat4_mul(gfx3d_camera_projection(cam), gfx3d_camera_view(cam));
}

void gfx3d_camera_apply(const Gfx3dCamera *cam) {
    g_active_view = gfx3d_camera_view(cam);
#ifdef CALYNDA_WII_BUILD
    Mtx view;
    Mtx44 proj;

    guVector eye = { cam->position.x, cam->position.y, cam->position.z };
    guVector look = { cam->target.x, cam->target.y, cam->target.z };
    guVector up = { cam->up.x, cam->up.y, cam->up.z };

    guLookAt(view, &eye, &up, &look);
    GX_LoadPosMtxImm(view, GX_PNMTX0);

    /* Load normal matrix for lighting (inverse transpose of the 3x3) */
    Mtx nrm;
    guMtxInverse(view, nrm);
    guMtxTranspose(nrm, nrm);
    GX_LoadNrmMtxImm(nrm, GX_PNMTX0);

    guPerspective(proj, cam->fov_y, cam->aspect, cam->near_z, cam->far_z);
    GX_LoadProjectionMtx(proj, GX_PERSPECTIVE);
#else
    (void)cam;
#endif
}

void gfx3d_camera_orbit_rotate(Gfx3dCamera *cam, float delta_yaw, float delta_pitch) {
    cam->orbit_yaw += delta_yaw;
    cam->orbit_pitch += delta_pitch;

    /* Clamp pitch to avoid gimbal lock */
    if (cam->orbit_pitch > CM_PI * 0.45f) cam->orbit_pitch = CM_PI * 0.45f;
    if (cam->orbit_pitch < -CM_PI * 0.45f) cam->orbit_pitch = -CM_PI * 0.45f;

    /* Recompute position from spherical coords */
    float cp = cosf(cam->orbit_pitch);
    cam->position.x = cam->target.x + cam->orbit_distance * sinf(cam->orbit_yaw) * cp;
    cam->position.y = cam->target.y + cam->orbit_distance * sinf(cam->orbit_pitch);
    cam->position.z = cam->target.z + cam->orbit_distance * cosf(cam->orbit_yaw) * cp;
}

void gfx3d_camera_orbit_zoom(Gfx3dCamera *cam, float delta_distance) {
    cam->orbit_distance += delta_distance;
    if (cam->orbit_distance < 1.0f) cam->orbit_distance = 1.0f;
    if (cam->orbit_distance > 500.0f) cam->orbit_distance = 500.0f;

    /* Re-apply orbit */
    gfx3d_camera_orbit_rotate(cam, 0.0f, 0.0f);
}

void gfx3d_camera_follow_update(Gfx3dCamera *cam, CmVec3 target_pos) {
    cam->target = target_pos;
    CmVec3 desired = cm_vec3_add(target_pos, cam->follow_offset);
    cam->position = cm_vec3_lerp(cam->position, desired, cam->follow_speed);
}

void gfx3d_camera_freelook_move(Gfx3dCamera *cam, CmVec3 delta) {
    /* Move in camera-local space */
    CmVec3 forward = cm_vec3_normalize(cm_vec3_sub(cam->target, cam->position));
    CmVec3 right = cm_vec3_normalize(cm_vec3_cross(forward, cam->up));
    CmVec3 up = cm_vec3_cross(right, forward);

    CmVec3 move = cm_vec3_add(
        cm_vec3_add(cm_vec3_scale(right, delta.x), cm_vec3_scale(up, delta.y)),
        cm_vec3_scale(forward, delta.z)
    );
    cam->position = cm_vec3_add(cam->position, move);
    cam->target   = cm_vec3_add(cam->target, move);
}

void gfx3d_camera_freelook_rotate(Gfx3dCamera *cam, float delta_yaw, float delta_pitch) {
    CmVec3 dir = cm_vec3_sub(cam->target, cam->position);
    float dist = cm_vec3_length(dir);
    if (dist < CM_EPSILON) return;

    dir = cm_vec3_scale(dir, 1.0f / dist);

    /* Apply yaw (around world Y) */
    CmQuat yaw_q = cm_quat_from_axis_angle(cm_vec3(0, 1, 0), delta_yaw);
    dir = cm_quat_rotate_vec3(yaw_q, dir);

    /* Apply pitch (around camera-local right axis) */
    CmVec3 right = cm_vec3_normalize(cm_vec3_cross(dir, cam->up));
    CmQuat pitch_q = cm_quat_from_axis_angle(right, delta_pitch);
    dir = cm_quat_rotate_vec3(pitch_q, dir);

    /* Prevent flipping past vertical */
    if (cm_absf(cm_vec3_dot(dir, cm_vec3(0, 1, 0))) > 0.99f) return;

    cam->target = cm_vec3_add(cam->position, cm_vec3_scale(dir, dist));
}
