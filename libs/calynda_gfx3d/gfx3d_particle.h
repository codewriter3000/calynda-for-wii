/*
 * gfx3d_particle.h — Pool-allocated billboard particle system.
 *
 * Camera-facing quads for splashes, sparks, dust, ball trails.
 * No dynamic allocation after init.
 */

#ifndef GFX3D_PARTICLE_H
#define GFX3D_PARTICLE_H

#include "../calynda_math/calynda_math.h"
#include "gfx3d_texture.h"

#include <stdbool.h>
#include <stdint.h>

#define GFX3D_MAX_PARTICLES 512

typedef struct {
    CmVec3  position;
    CmVec3  velocity;
    float   life;           /* remaining life (seconds) */
    float   max_life;
    float   size;
    float   size_end;       /* size at death (for shrinking) */
    uint8_t color[4];       /* start color RGBA */
    uint8_t color_end[4];   /* end color RGBA */
    bool    active;
} Gfx3dParticle;

/* Emitter configuration */
typedef struct {
    CmVec3  position;
    CmVec3  emit_dir;        /* base emission direction */
    float   spread;           /* cone half-angle (radians) */
    float   speed_min;
    float   speed_max;
    float   life_min;
    float   life_max;
    float   size_start;
    float   size_end;
    uint8_t color_start[4];
    uint8_t color_end[4];
    float   gravity;          /* gravity multiplier (default 1.0) */
    int     emit_rate;        /* particles per second */
    Gfx3dTexture *texture;
} Gfx3dParticleEmitter;

/* Particle system (pool-allocated) */
typedef struct {
    Gfx3dParticle  particles[GFX3D_MAX_PARTICLES];
    int            active_count;
    float          emit_accumulator;
} Gfx3dParticleSystem;

/* Initialize particle system (zeroes pool) */
void gfx3d_particles_init(Gfx3dParticleSystem *sys);

/* Emit a burst of particles from an emitter */
void gfx3d_particles_emit(Gfx3dParticleSystem *sys,
                            const Gfx3dParticleEmitter *emitter,
                            int count);

/* Update all particles (call once per frame with dt in seconds) */
void gfx3d_particles_update(Gfx3dParticleSystem *sys, float dt,
                              const Gfx3dParticleEmitter *emitter);

/*
 * Draw all active particles as camera-facing billboards.
 * cam_right / cam_up: camera basis vectors for billboarding.
 */
void gfx3d_particles_draw(const Gfx3dParticleSystem *sys,
                            CmVec3 cam_right, CmVec3 cam_up,
                            Gfx3dTexture *tex);

/* Create a default emitter for common effects */
Gfx3dParticleEmitter gfx3d_emitter_splash(CmVec3 pos);
Gfx3dParticleEmitter gfx3d_emitter_sparks(CmVec3 pos);
Gfx3dParticleEmitter gfx3d_emitter_dust(CmVec3 pos);
Gfx3dParticleEmitter gfx3d_emitter_trail(CmVec3 pos);

#endif /* GFX3D_PARTICLE_H */
