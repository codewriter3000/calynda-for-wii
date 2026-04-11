/*
 * gfx3d_water.h — Water surface rendering with scrolling UV and
 * optional sinusoidal vertex displacement.
 */

#ifndef GFX3D_WATER_H
#define GFX3D_WATER_H

#include "../calynda_math/calynda_math.h"
#include "gfx3d_texture.h"

#include <stdbool.h>

typedef struct {
    float   width;
    float   depth;
    float   y_level;          /* water plane Y height */
    int     subdivisions;     /* grid resolution */

    /* UV scrolling */
    float   uv_speed_x;
    float   uv_speed_z;
    float   uv_offset_x;     /* accumulated scroll offset */
    float   uv_offset_z;

    /* Vertex displacement (waves) */
    bool    displacement_enabled;
    float   wave_amplitude;
    float   wave_frequency;
    float   wave_speed;

    /* Appearance */
    Gfx3dTexture *texture;
    uint8_t color[4];         /* tint RGBA */
    float   alpha;

    /* Internal mesh (rebuilt each frame if displaced) */
    void   *mesh_data;        /* opaque; managed internally */
} Gfx3dWater;

/* Create a water plane */
Gfx3dWater gfx3d_water_create(float width, float depth, float y_level,
                                int subdivisions);

/* Set the water texture */
void gfx3d_water_set_texture(Gfx3dWater *water, Gfx3dTexture *tex);

/* Update water animation (call once per frame) */
void gfx3d_water_update(Gfx3dWater *water, float dt, float time);

/* Draw the water surface */
void gfx3d_water_draw(const Gfx3dWater *water);

/* Clean up water resources */
void gfx3d_water_destroy(Gfx3dWater *water);

#endif /* GFX3D_WATER_H */
