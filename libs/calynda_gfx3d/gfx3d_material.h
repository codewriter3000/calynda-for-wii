/*
 * gfx3d_material.h — Material system combining texture + color + TEV mode.
 */

#ifndef GFX3D_MATERIAL_H
#define GFX3D_MATERIAL_H

#include "gfx3d_texture.h"

#include <stdbool.h>
#include <stdint.h>

/* TEV mode presets for common rendering styles */
typedef enum {
    GFX3D_MAT_LIT,           /* Textured + lit (modulate tex × light) */
    GFX3D_MAT_UNLIT,         /* Textured, no lighting */
    GFX3D_MAT_COLOR_ONLY,    /* Vertex color only, no texture */
    GFX3D_MAT_TRANSPARENT,   /* Textured + alpha blending */
    GFX3D_MAT_ADDITIVE       /* Additive blending (particles/glow) */
} Gfx3dMaterialMode;

typedef struct {
    Gfx3dTexture       *texture;      /* NULL = use color_only mode */
    uint8_t             color[4];     /* base tint RGBA */
    float               alpha;        /* overall alpha (0..1) */
    Gfx3dMaterialMode   mode;
    bool                double_sided; /* disable face culling */
    bool                depth_write;  /* write to z-buffer */
    bool                depth_test;   /* test against z-buffer */
} Gfx3dMaterial;

/* Create default opaque white lit material */
Gfx3dMaterial gfx3d_material_default(void);

/* Create a material with a texture */
Gfx3dMaterial gfx3d_material_textured(Gfx3dTexture *tex);

/* Create a flat-color (untextured) material */
Gfx3dMaterial gfx3d_material_flat_color(uint8_t r, uint8_t g, uint8_t b, uint8_t a);

/* Create a transparent material */
Gfx3dMaterial gfx3d_material_transparent(Gfx3dTexture *tex, float alpha);

/* Create an additive material (for particles) */
Gfx3dMaterial gfx3d_material_additive(Gfx3dTexture *tex);

/*
 * Apply material to the GX pipeline (sets TEV stages, blend mode,
 * cull mode, depth state, binds texture).
 */
void gfx3d_material_apply(const Gfx3dMaterial *mat);

#endif /* GFX3D_MATERIAL_H */
