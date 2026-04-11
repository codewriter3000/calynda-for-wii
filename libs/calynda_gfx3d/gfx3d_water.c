/*
 * gfx3d_water.c — Water surface rendering implementation.
 */

#include "gfx3d_water.h"
#include "gfx3d_mesh.h"
#include "gfx3d_material.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#ifdef CALYNDA_WII_BUILD
#include <gccore.h>
#endif

Gfx3dWater gfx3d_water_create(float width, float depth, float y_level,
                                int subdivisions) {
    Gfx3dWater w;
    memset(&w, 0, sizeof(w));
    w.width = width;
    w.depth = depth;
    w.y_level = y_level;
    w.subdivisions = subdivisions;
    w.uv_speed_x = 0.02f;
    w.uv_speed_z = 0.015f;
    w.displacement_enabled = true;
    w.wave_amplitude = 0.3f;
    w.wave_frequency = 2.0f;
    w.wave_speed = 1.5f;
    w.color[0] = 100; w.color[1] = 150; w.color[2] = 220; w.color[3] = 180;
    w.alpha = 0.7f;
    return w;
}

void gfx3d_water_set_texture(Gfx3dWater *water, Gfx3dTexture *tex) {
    water->texture = tex;
}

void gfx3d_water_update(Gfx3dWater *water, float dt, float time) {
    (void)time;
    water->uv_offset_x += water->uv_speed_x * dt;
    water->uv_offset_z += water->uv_speed_z * dt;
    /* Keep UV offsets from growing unbounded */
    if (water->uv_offset_x > 1.0f) water->uv_offset_x -= 1.0f;
    if (water->uv_offset_z > 1.0f) water->uv_offset_z -= 1.0f;
}

void gfx3d_water_draw(const Gfx3dWater *water) {
#ifdef CALYNDA_WII_BUILD
    int sub = water->subdivisions;
    if (sub < 1) sub = 1;
    int cols = sub + 1;
    int rows = sub + 1;

    /* Semi-transparent setup */
    GX_SetZMode(GX_TRUE, GX_LEQUAL, GX_FALSE); /* read depth, no write */
    GX_SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR);
    GX_SetCullMode(GX_CULL_NONE);

    /* Disable lighting for water — use PASSCLR or MODULATE with texture */
    GX_SetChanCtrl(GX_COLOR0A0, GX_DISABLE, GX_SRC_VTX, GX_SRC_VTX,
                    GX_LIGHTNULL, GX_DF_NONE, GX_AF_NONE);

    if (water->texture) {
        gfx3d_texture_bind(water->texture, GX_TEXMAP0);
        GX_SetTevOp(GX_TEVSTAGE0, GX_MODULATE);
        GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
    } else {
        GX_SetTevOp(GX_TEVSTAGE0, GX_PASSCLR);
        GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORDNULL, GX_TEXMAP_NULL, GX_COLOR0A0);
    }

    CmMat4 ident = cm_mat4_identity();
    gfx3d_load_model_matrix(ident);

    /* Draw as triangle strip rows */
    float hw = water->width * 0.5f;
    float hd = water->depth * 0.5f;

    for (int z = 0; z < sub; z++) {
        GX_Begin(GX_TRIANGLESTRIP, GX_VTXFMT0, cols * 2);
        for (int x = 0; x < cols; x++) {
            for (int dz = 1; dz >= 0; dz--) {
                float px = -hw + water->width * (float)x / (float)sub;
                float pz = -hd + water->depth * (float)(z + dz) / (float)sub;
                float py = water->y_level;

                if (water->displacement_enabled) {
                    py += water->wave_amplitude * sinf(
                        px * water->wave_frequency +
                        pz * water->wave_frequency * 0.7f +
                        water->uv_offset_x * 100.0f * water->wave_speed);
                }

                float u = (float)x / (float)sub + water->uv_offset_x;
                float v = (float)(z + dz) / (float)sub + water->uv_offset_z;

                GX_Position3f32(px, py, pz);
                GX_Normal3f32(0, 1, 0);
                GX_TexCoord2f32(u, v);
                GX_Color4u8(water->color[0], water->color[1],
                            water->color[2], (uint8_t)(water->alpha * 255));
            }
        }
        GX_End();
    }

    /* Restore state */
    GX_SetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
    GX_SetCullMode(GX_CULL_BACK);
#else
    (void)water;
#endif
}

void gfx3d_water_destroy(Gfx3dWater *water) {
    if (water->mesh_data) {
        free(water->mesh_data);
        water->mesh_data = NULL;
    }
}
