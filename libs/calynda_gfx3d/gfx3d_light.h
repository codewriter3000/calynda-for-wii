/*
 * gfx3d_light.h — Hardware lighting wrapping GX (up to 8 lights).
 */

#ifndef GFX3D_LIGHT_H
#define GFX3D_LIGHT_H

#include "../calynda_math/calynda_math.h"
#include "gfx3d_init.h"

#include <stdint.h>

#define GFX3D_MAX_LIGHTS 8

typedef enum {
    GFX3D_LIGHT_DIRECTIONAL,
    GFX3D_LIGHT_POINT,
    GFX3D_LIGHT_SPOT
} Gfx3dLightType;

typedef struct {
    Gfx3dLightType type;
    CmVec3         position;      /* world-space position (point/spot) */
    CmVec3         direction;     /* direction (directional/spot) */
    Gfx3dColor     color;
    float          intensity;     /* brightness multiplier */

    /* Attenuation (point/spot): 1 / (k0 + k1*d + k2*d^2) */
    float          atten_k0;     /* constant */
    float          atten_k1;     /* linear */
    float          atten_k2;     /* quadratic */

    /* Spot: cutoff angle in degrees */
    float          spot_cutoff;
    bool           active;
} Gfx3dLight;

/* Ambient light color */
typedef struct {
    uint8_t r, g, b;
} Gfx3dAmbient;

/* Create a directional light (like the sun) */
Gfx3dLight gfx3d_light_directional(CmVec3 direction, Gfx3dColor color, float intensity);

/* Create a point light */
Gfx3dLight gfx3d_light_point(CmVec3 position, Gfx3dColor color, float intensity, float range);

/* Set the ambient light level */
void gfx3d_set_ambient(Gfx3dAmbient ambient);

/*
 * Upload lights to GX hardware. Must be called each frame after
 * the view matrix is set so light positions are in view-space.
 * view: the current camera view matrix.
 */
void gfx3d_lights_apply(const Gfx3dLight *lights, int count, CmMat4 view);

#endif /* GFX3D_LIGHT_H */
