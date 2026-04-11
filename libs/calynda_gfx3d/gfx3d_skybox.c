/*
 * gfx3d_skybox.c — 6-face cubemap skybox rendering.
 */

#include "gfx3d_skybox.h"
#include "gfx3d_mesh.h"

#ifdef CALYNDA_WII_BUILD
#include <gccore.h>
#endif

Gfx3dSkybox gfx3d_skybox_create(Gfx3dTexture *px, Gfx3dTexture *nx,
                                  Gfx3dTexture *py, Gfx3dTexture *ny,
                                  Gfx3dTexture *pz, Gfx3dTexture *nz,
                                  float size) {
    Gfx3dSkybox sky;
    sky.faces[0] = px; sky.faces[1] = nx;
    sky.faces[2] = py; sky.faces[3] = ny;
    sky.faces[4] = pz; sky.faces[5] = nz;
    sky.size = size;
    return sky;
}

void gfx3d_skybox_draw(const Gfx3dSkybox *skybox, CmVec3 camera_pos) {
#ifdef CALYNDA_WII_BUILD
    float s = skybox->size;

    /* Disable depth writes, keep depth test off */
    GX_SetZMode(GX_FALSE, GX_LEQUAL, GX_FALSE);
    GX_SetCullMode(GX_CULL_NONE);

    /* Set TEV to replace with texture */
    GX_SetNumTevStages(1);
    GX_SetTevOp(GX_TEVSTAGE0, GX_REPLACE);
    GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);

    /* Disable lighting for skybox */
    GX_SetChanCtrl(GX_COLOR0A0, GX_DISABLE, GX_SRC_VTX, GX_SRC_VTX,
                    GX_LIGHTNULL, GX_DF_NONE, GX_AF_NONE);

    /* Load model matrix centered on camera */
    CmMat4 model = cm_mat4_translate(camera_pos);
    gfx3d_load_model_matrix(model);

    /* Face vertices: 6 quads, each with 4 verts
       Order: +X, -X, +Y, -Y, +Z, -Z */
    struct { float x, y, z, u, v; } face_verts[6][4] = {
        /* +X */ {{ s,-s,-s, 0,1},{ s,-s, s, 1,1},{ s, s, s, 1,0},{ s, s,-s, 0,0}},
        /* -X */ {{-s,-s, s, 0,1},{-s,-s,-s, 1,1},{-s, s,-s, 1,0},{-s, s, s, 0,0}},
        /* +Y */ {{-s, s, s, 0,1},{ s, s, s, 1,1},{ s, s,-s, 1,0},{-s, s,-s, 0,0}},
        /* -Y */ {{-s,-s,-s, 0,1},{ s,-s,-s, 1,1},{ s,-s, s, 1,0},{-s,-s, s, 0,0}},
        /* +Z */ {{ s,-s, s, 0,1},{-s,-s, s, 1,1},{-s, s, s, 1,0},{ s, s, s, 0,0}},
        /* -Z */ {{-s,-s,-s, 0,1},{ s,-s,-s, 1,1},{ s, s,-s, 1,0},{-s, s,-s, 0,0}},
    };

    for (int f = 0; f < 6; f++) {
        if (!skybox->faces[f]) continue;
        gfx3d_texture_bind(skybox->faces[f], GX_TEXMAP0);

        GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
        for (int v = 0; v < 4; v++) {
            GX_Position3f32(face_verts[f][v].x, face_verts[f][v].y, face_verts[f][v].z);
            GX_Normal3f32(0, 0, 0);
            GX_TexCoord2f32(face_verts[f][v].u, face_verts[f][v].v);
            GX_Color4u8(255, 255, 255, 255);
        }
        GX_End();
    }

    /* Restore depth mode */
    GX_SetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
    GX_SetCullMode(GX_CULL_BACK);
#else
    (void)skybox; (void)camera_pos;
#endif
}
