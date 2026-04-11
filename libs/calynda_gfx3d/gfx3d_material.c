/*
 * gfx3d_material.c — Material system implementation.
 */

#include "gfx3d_material.h"

#include <stddef.h>

#ifdef CALYNDA_WII_BUILD
#include <gccore.h>
#endif

Gfx3dMaterial gfx3d_material_default(void) {
    Gfx3dMaterial mat;
    mat.texture = NULL;
    mat.color[0] = mat.color[1] = mat.color[2] = mat.color[3] = 255;
    mat.alpha = 1.0f;
    mat.mode = GFX3D_MAT_LIT;
    mat.double_sided = false;
    mat.depth_write = true;
    mat.depth_test = true;
    return mat;
}

Gfx3dMaterial gfx3d_material_textured(Gfx3dTexture *tex) {
    Gfx3dMaterial mat = gfx3d_material_default();
    mat.texture = tex;
    return mat;
}

Gfx3dMaterial gfx3d_material_flat_color(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    Gfx3dMaterial mat = gfx3d_material_default();
    mat.color[0] = r; mat.color[1] = g; mat.color[2] = b; mat.color[3] = a;
    mat.mode = GFX3D_MAT_COLOR_ONLY;
    return mat;
}

Gfx3dMaterial gfx3d_material_transparent(Gfx3dTexture *tex, float alpha) {
    Gfx3dMaterial mat = gfx3d_material_default();
    mat.texture = tex;
    mat.alpha = alpha;
    mat.mode = GFX3D_MAT_TRANSPARENT;
    mat.depth_write = false;
    return mat;
}

Gfx3dMaterial gfx3d_material_additive(Gfx3dTexture *tex) {
    Gfx3dMaterial mat = gfx3d_material_default();
    mat.texture = tex;
    mat.mode = GFX3D_MAT_ADDITIVE;
    mat.depth_write = false;
    return mat;
}

void gfx3d_material_apply(const Gfx3dMaterial *mat) {
#ifdef CALYNDA_WII_BUILD
    /* Cull mode */
    GX_SetCullMode(mat->double_sided ? GX_CULL_NONE : GX_CULL_BACK);

    /* Depth */
    GX_SetZMode(mat->depth_test ? GX_TRUE : GX_FALSE, GX_LEQUAL,
                mat->depth_write ? GX_TRUE : GX_FALSE);

    /* Blend mode */
    switch (mat->mode) {
        case GFX3D_MAT_TRANSPARENT:
            GX_SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR);
            break;
        case GFX3D_MAT_ADDITIVE:
            GX_SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_ONE, GX_LO_CLEAR);
            break;
        default:
            GX_SetBlendMode(GX_BM_NONE, GX_BL_ONE, GX_BL_ZERO, GX_LO_CLEAR);
            break;
    }

    /* TEV configuration */
    GX_SetNumTevStages(1);
    if (mat->texture && mat->texture->loaded) {
        gfx3d_texture_bind(mat->texture, GX_TEXMAP0);
        GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);

        if (mat->mode == GFX3D_MAT_UNLIT || mat->mode == GFX3D_MAT_ADDITIVE) {
            GX_SetTevOp(GX_TEVSTAGE0, GX_REPLACE);
        } else {
            GX_SetTevOp(GX_TEVSTAGE0, GX_MODULATE);
        }
    } else {
        /* No texture: pass through vertex color */
        GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORDNULL, GX_TEXMAP_NULL, GX_COLOR0A0);
        GX_SetTevOp(GX_TEVSTAGE0, GX_PASSCLR);
    }

    /* Channel control for lighting */
    if (mat->mode == GFX3D_MAT_LIT) {
        GX_SetChanCtrl(GX_COLOR0A0, GX_ENABLE, GX_SRC_REG, GX_SRC_VTX,
                        GX_LIGHT0 | GX_LIGHT1 | GX_LIGHT2, GX_DF_CLAMP, GX_AF_SPOT);
    } else {
        GX_SetChanCtrl(GX_COLOR0A0, GX_DISABLE, GX_SRC_VTX, GX_SRC_VTX,
                        GX_LIGHTNULL, GX_DF_NONE, GX_AF_NONE);
    }
#else
    (void)mat;
#endif
}
