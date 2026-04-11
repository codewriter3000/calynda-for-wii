/*
 * gfx3d_camera.h — Camera system with follow-cam, orbit, and free-look.
 */

#ifndef GFX3D_CAMERA_H
#define GFX3D_CAMERA_H

#include "../calynda_math/calynda_math.h"

typedef enum {
    GFX3D_CAM_FREE,
    GFX3D_CAM_ORBIT,
    GFX3D_CAM_FOLLOW
} Gfx3dCameraMode;

typedef struct {
    CmVec3  position;
    CmVec3  target;
    CmVec3  up;

    float   fov_y;       /* degrees */
    float   near_z;
    float   far_z;
    float   aspect;

    Gfx3dCameraMode mode;

    /* Orbit camera state */
    float   orbit_distance;
    float   orbit_yaw;      /* radians */
    float   orbit_pitch;    /* radians */

    /* Follow camera state */
    CmVec3  follow_offset;  /* offset from target in local space */
    float   follow_speed;   /* lerp speed (0..1 per frame) */
} Gfx3dCamera;

/* Create a default camera */
Gfx3dCamera gfx3d_camera_default(float aspect);

/* Get the view matrix for the camera */
CmMat4 gfx3d_camera_view(const Gfx3dCamera *cam);

/* Get the projection matrix for the camera */
CmMat4 gfx3d_camera_projection(const Gfx3dCamera *cam);

/* Get combined view-projection matrix */
CmMat4 gfx3d_camera_view_projection(const Gfx3dCamera *cam);

/*
 * Load camera matrices into GX hardware.
 * On host builds this is a no-op.
 */
void gfx3d_camera_apply(const Gfx3dCamera *cam);

/* Get the view matrix from the last camera_apply call */
CmMat4 gfx3d_camera_active_view(void);

/* Update orbit camera from input deltas (yaw/pitch in radians) */
void gfx3d_camera_orbit_rotate(Gfx3dCamera *cam, float delta_yaw, float delta_pitch);

/* Update orbit camera zoom */
void gfx3d_camera_orbit_zoom(Gfx3dCamera *cam, float delta_distance);

/* Update follow camera toward a target position each frame */
void gfx3d_camera_follow_update(Gfx3dCamera *cam, CmVec3 target_pos);

/* Free-look: move camera by position delta */
void gfx3d_camera_freelook_move(Gfx3dCamera *cam, CmVec3 delta);

/* Free-look: rotate by yaw/pitch deltas (radians) */
void gfx3d_camera_freelook_rotate(Gfx3dCamera *cam, float delta_yaw, float delta_pitch);

#endif /* GFX3D_CAMERA_H */
