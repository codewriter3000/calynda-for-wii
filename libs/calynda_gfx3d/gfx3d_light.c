/*
 * gfx3d_light.c — GX hardware lighting implementation.
 */

#include "gfx3d_light.h"

#ifdef CALYNDA_WII_BUILD
#include <gccore.h>
#endif

Gfx3dLight gfx3d_light_directional(CmVec3 direction, Gfx3dColor color, float intensity) {
    Gfx3dLight l;
    l.type = GFX3D_LIGHT_DIRECTIONAL;
    l.position = cm_vec3_zero();
    l.direction = cm_vec3_normalize(direction);
    l.color = color;
    l.intensity = intensity;
    l.atten_k0 = 1.0f;
    l.atten_k1 = 0.0f;
    l.atten_k2 = 0.0f;
    l.spot_cutoff = 0.0f;
    l.active = true;
    return l;
}

Gfx3dLight gfx3d_light_point(CmVec3 position, Gfx3dColor color, float intensity, float range) {
    Gfx3dLight l;
    l.type = GFX3D_LIGHT_POINT;
    l.position = position;
    l.direction = cm_vec3(0, -1, 0);
    l.color = color;
    l.intensity = intensity;
    /* Set attenuation so light fades to ~0 at 'range' */
    l.atten_k0 = 1.0f;
    l.atten_k1 = 2.0f / range;
    l.atten_k2 = 1.0f / (range * range);
    l.spot_cutoff = 0.0f;
    l.active = true;
    return l;
}

void gfx3d_set_ambient(Gfx3dAmbient ambient) {
#ifdef CALYNDA_WII_BUILD
    GXColor amb = { ambient.r, ambient.g, ambient.b, 255 };
    GX_SetChanAmbColor(GX_COLOR0A0, amb);
#else
    (void)ambient;
#endif
}

void gfx3d_lights_apply(const Gfx3dLight *lights, int count, CmMat4 view) {
#ifdef CALYNDA_WII_BUILD
    if (count > GFX3D_MAX_LIGHTS) count = GFX3D_MAX_LIGHTS;

    uint8_t light_mask = 0;
    for (int i = 0; i < count; i++) {
        if (!lights[i].active) continue;

        GXLightObj lobj;
        uint8_t scaled_r = (uint8_t)(lights[i].color.r * cm_clampf(lights[i].intensity, 0.0f, 1.0f));
        uint8_t scaled_g = (uint8_t)(lights[i].color.g * cm_clampf(lights[i].intensity, 0.0f, 1.0f));
        uint8_t scaled_b = (uint8_t)(lights[i].color.b * cm_clampf(lights[i].intensity, 0.0f, 1.0f));
        GXColor lcol = { scaled_r, scaled_g, scaled_b, 255 };
        GX_InitLightColor(&lobj, lcol);

        if (lights[i].type == GFX3D_LIGHT_DIRECTIONAL) {
            /* For directional lights, position at infinity along -direction */
            CmVec3 far_pos = cm_vec3_scale(cm_vec3_neg(lights[i].direction), 100000.0f);
            CmVec3 view_pos = cm_mat4_mul_point(view, far_pos);
            guVector gpos = { view_pos.x, view_pos.y, view_pos.z };
            GX_InitLightPos(&lobj, gpos.x, gpos.y, gpos.z);
            GX_InitLightAttn(&lobj, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
        } else {
            CmVec3 view_pos = cm_mat4_mul_point(view, lights[i].position);
            guVector gpos = { view_pos.x, view_pos.y, view_pos.z };
            GX_InitLightPos(&lobj, gpos.x, gpos.y, gpos.z);
            GX_InitLightAttn(&lobj, 1.0f, 0.0f, 0.0f,
                              lights[i].atten_k0, lights[i].atten_k1, lights[i].atten_k2);
        }

        GX_LoadLightObj(&lobj, 1 << i);
        light_mask |= (1 << i);
    }

    /* Configure channel to use these lights */
    GX_SetNumChans(1);
    GX_SetChanCtrl(GX_COLOR0A0, GX_ENABLE, GX_SRC_REG, GX_SRC_VTX,
                    light_mask, GX_DF_CLAMP, GX_AF_SPOT);
#else
    (void)lights; (void)count; (void)view;
#endif
}
