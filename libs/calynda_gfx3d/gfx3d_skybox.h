/*
 * gfx3d_skybox.h — 6-face cubemap skybox, rendered first with depth write off.
 */

#ifndef GFX3D_SKYBOX_H
#define GFX3D_SKYBOX_H

#include "gfx3d_texture.h"
#include "../calynda_math/calynda_math.h"

typedef struct {
    Gfx3dTexture *faces[6];   /* +X,-X,+Y,-Y,+Z,-Z */
    float         size;
} Gfx3dSkybox;

/* Create skybox from 6 texture pointers */
Gfx3dSkybox gfx3d_skybox_create(Gfx3dTexture *px, Gfx3dTexture *nx,
                                  Gfx3dTexture *py, Gfx3dTexture *ny,
                                  Gfx3dTexture *pz, Gfx3dTexture *nz,
                                  float size);

/*
 * Draw the skybox. Should be called FIRST in the frame, before
 * any other 3D objects. Temporarily disables depth writes.
 * camera_pos: skybox is centered on the camera so it doesn't move.
 */
void gfx3d_skybox_draw(const Gfx3dSkybox *skybox, CmVec3 camera_pos);

#endif /* GFX3D_SKYBOX_H */
