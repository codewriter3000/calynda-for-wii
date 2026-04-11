/*
 * gfx3d_particle.c — Billboard particle system implementation.
 */

#include "gfx3d_particle.h"
#include "gfx3d_material.h"
#include "gfx3d_mesh.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#ifdef CALYNDA_WII_BUILD
#include <gccore.h>
#endif

/* Simple deterministic pseudo-random for particle variation */
static uint32_t particle_rng_state = 12345;
static float particle_randf(void) {
    particle_rng_state = particle_rng_state * 1103515245 + 12345;
    return (float)(particle_rng_state >> 16 & 0x7FFF) / 32767.0f;
}

static float particle_randf_range(float lo, float hi) {
    return lo + particle_randf() * (hi - lo);
}

void gfx3d_particles_init(Gfx3dParticleSystem *sys) {
    memset(sys, 0, sizeof(Gfx3dParticleSystem));
}

void gfx3d_particles_emit(Gfx3dParticleSystem *sys,
                            const Gfx3dParticleEmitter *emitter,
                            int count) {
    for (int n = 0; n < count; n++) {
        /* Find an inactive slot */
        int slot = -1;
        for (int i = 0; i < GFX3D_MAX_PARTICLES; i++) {
            if (!sys->particles[i].active) {
                slot = i;
                break;
            }
        }
        if (slot < 0) return; /* pool exhausted */

        Gfx3dParticle *p = &sys->particles[slot];
        p->active = true;
        p->position = emitter->position;

        /* Random direction within cone */
        float theta = particle_randf() * 2.0f * CM_PI;
        float phi = particle_randf() * emitter->spread;
        float sp = sinf(phi);
        CmVec3 dir = cm_vec3(sp * cosf(theta), cosf(phi), sp * sinf(theta));

        /* Rotate cone to match emit_dir */
        CmVec3 up = cm_vec3(0, 1, 0);
        CmVec3 target = cm_vec3_normalize(emitter->emit_dir);
        if (cm_absf(cm_vec3_dot(up, target)) < 0.999f) {
            CmVec3 axis = cm_vec3_cross(up, target);
            float angle = acosf(cm_clampf(cm_vec3_dot(up, target), -1.0f, 1.0f));
            CmQuat q = cm_quat_from_axis_angle(axis, angle);
            dir = cm_quat_rotate_vec3(q, dir);
        } else if (cm_vec3_dot(up, target) < 0) {
            dir = cm_vec3_neg(dir);
        }

        float speed = particle_randf_range(emitter->speed_min, emitter->speed_max);
        p->velocity = cm_vec3_scale(dir, speed);
        p->life = particle_randf_range(emitter->life_min, emitter->life_max);
        p->max_life = p->life;
        p->size = emitter->size_start;
        p->size_end = emitter->size_end;
        memcpy(p->color, emitter->color_start, 4);
        memcpy(p->color_end, emitter->color_end, 4);

        sys->active_count++;
    }
}

void gfx3d_particles_update(Gfx3dParticleSystem *sys, float dt,
                              const Gfx3dParticleEmitter *emitter) {
    /* Auto-emit based on rate */
    if (emitter && emitter->emit_rate > 0) {
        sys->emit_accumulator += dt * emitter->emit_rate;
        while (sys->emit_accumulator >= 1.0f) {
            gfx3d_particles_emit(sys, emitter, 1);
            sys->emit_accumulator -= 1.0f;
        }
    }

    float gravity = emitter ? emitter->gravity : 1.0f;
    CmVec3 gravity_vec = cm_vec3(0, -9.81f * gravity * dt, 0);

    for (int i = 0; i < GFX3D_MAX_PARTICLES; i++) {
        Gfx3dParticle *p = &sys->particles[i];
        if (!p->active) continue;

        p->life -= dt;
        if (p->life <= 0.0f) {
            p->active = false;
            sys->active_count--;
            continue;
        }

        /* Integrate */
        p->velocity = cm_vec3_add(p->velocity, gravity_vec);
        p->position = cm_vec3_add(p->position, cm_vec3_scale(p->velocity, dt));

        /* Interpolate size and color based on life fraction */
        float t = 1.0f - (p->life / p->max_life);
        p->size = cm_lerpf(p->size, p->size_end, t);
        for (int c = 0; c < 4; c++) {
            p->color[c] = (uint8_t)(p->color[c] + (int)(p->color_end[c] - p->color[c]) * t);
        }
    }
}

void gfx3d_particles_draw(const Gfx3dParticleSystem *sys,
                            CmVec3 cam_right, CmVec3 cam_up,
                            Gfx3dTexture *tex) {
#ifdef CALYNDA_WII_BUILD
    /* Set up for additive blended billboards */
    GX_SetZMode(GX_TRUE, GX_LEQUAL, GX_FALSE); /* read depth, don't write */
    GX_SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_ONE, GX_LO_CLEAR);
    GX_SetCullMode(GX_CULL_NONE);

    if (tex) {
        gfx3d_texture_bind(tex, GX_TEXMAP0);
        GX_SetTevOp(GX_TEVSTAGE0, GX_MODULATE);
        GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
    } else {
        GX_SetTevOp(GX_TEVSTAGE0, GX_PASSCLR);
        GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORDNULL, GX_TEXMAP_NULL, GX_COLOR0A0);
    }

    /* Load identity model matrix — positions are in world space */
    CmMat4 ident = cm_mat4_identity();
    gfx3d_load_model_matrix(ident);

    for (int i = 0; i < GFX3D_MAX_PARTICLES; i++) {
        const Gfx3dParticle *p = &sys->particles[i];
        if (!p->active) continue;

        float hs = p->size * 0.5f;
        CmVec3 r = cm_vec3_scale(cam_right, hs);
        CmVec3 u = cm_vec3_scale(cam_up, hs);

        CmVec3 bl = cm_vec3_sub(cm_vec3_sub(p->position, r), u);
        CmVec3 br = cm_vec3_sub(cm_vec3_add(p->position, r), u);
        CmVec3 tr = cm_vec3_add(cm_vec3_add(p->position, r), u);
        CmVec3 tl = cm_vec3_add(cm_vec3_sub(p->position, r), u);

        GX_Begin(GX_QUADS, GX_VTXFMT0, 4);

        GX_Position3f32(bl.x, bl.y, bl.z);
        GX_Normal3f32(0, 0, 1);
        GX_TexCoord2f32(0, 1);
        GX_Color4u8(p->color[0], p->color[1], p->color[2], p->color[3]);

        GX_Position3f32(br.x, br.y, br.z);
        GX_Normal3f32(0, 0, 1);
        GX_TexCoord2f32(1, 1);
        GX_Color4u8(p->color[0], p->color[1], p->color[2], p->color[3]);

        GX_Position3f32(tr.x, tr.y, tr.z);
        GX_Normal3f32(0, 0, 1);
        GX_TexCoord2f32(1, 0);
        GX_Color4u8(p->color[0], p->color[1], p->color[2], p->color[3]);

        GX_Position3f32(tl.x, tl.y, tl.z);
        GX_Normal3f32(0, 0, 1);
        GX_TexCoord2f32(0, 0);
        GX_Color4u8(p->color[0], p->color[1], p->color[2], p->color[3]);

        GX_End();
    }

    /* Restore state */
    GX_SetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
    GX_SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR);
    GX_SetCullMode(GX_CULL_BACK);
#else
    (void)sys; (void)cam_right; (void)cam_up; (void)tex;
#endif
}

/* ================================================================== */
/* Preset Emitters                                                      */
/* ================================================================== */

Gfx3dParticleEmitter gfx3d_emitter_splash(CmVec3 pos) {
    Gfx3dParticleEmitter e;
    memset(&e, 0, sizeof(e));
    e.position = pos;
    e.emit_dir = cm_vec3(0, 1, 0);
    e.spread = CM_PI * 0.35f;
    e.speed_min = 2.0f; e.speed_max = 5.0f;
    e.life_min = 0.3f;  e.life_max = 0.8f;
    e.size_start = 0.3f; e.size_end = 0.05f;
    e.color_start[0] = 180; e.color_start[1] = 220; e.color_start[2] = 255; e.color_start[3] = 200;
    e.color_end[0] = 200; e.color_end[1] = 230; e.color_end[2] = 255; e.color_end[3] = 0;
    e.gravity = 1.0f;
    e.emit_rate = 0; /* burst only */
    e.texture = NULL;
    return e;
}

Gfx3dParticleEmitter gfx3d_emitter_sparks(CmVec3 pos) {
    Gfx3dParticleEmitter e;
    memset(&e, 0, sizeof(e));
    e.position = pos;
    e.emit_dir = cm_vec3(0, 1, 0);
    e.spread = CM_PI * 0.5f;
    e.speed_min = 3.0f; e.speed_max = 8.0f;
    e.life_min = 0.2f;  e.life_max = 0.5f;
    e.size_start = 0.1f; e.size_end = 0.02f;
    e.color_start[0] = 255; e.color_start[1] = 200; e.color_start[2] = 50; e.color_start[3] = 255;
    e.color_end[0] = 255; e.color_end[1] = 100; e.color_end[2] = 0; e.color_end[3] = 0;
    e.gravity = 0.5f;
    e.emit_rate = 0;
    e.texture = NULL;
    return e;
}

Gfx3dParticleEmitter gfx3d_emitter_dust(CmVec3 pos) {
    Gfx3dParticleEmitter e;
    memset(&e, 0, sizeof(e));
    e.position = pos;
    e.emit_dir = cm_vec3(0, 1, 0);
    e.spread = CM_PI * 0.3f;
    e.speed_min = 0.5f; e.speed_max = 1.5f;
    e.life_min = 0.5f;  e.life_max = 1.5f;
    e.size_start = 0.2f; e.size_end = 0.5f;
    e.color_start[0] = 200; e.color_start[1] = 180; e.color_start[2] = 140; e.color_start[3] = 150;
    e.color_end[0] = 200; e.color_end[1] = 180; e.color_end[2] = 140; e.color_end[3] = 0;
    e.gravity = -0.1f; /* slight updraft */
    e.emit_rate = 0;
    e.texture = NULL;
    return e;
}

Gfx3dParticleEmitter gfx3d_emitter_trail(CmVec3 pos) {
    Gfx3dParticleEmitter e;
    memset(&e, 0, sizeof(e));
    e.position = pos;
    e.emit_dir = cm_vec3(0, 0, -1);
    e.spread = 0.05f;
    e.speed_min = 0.1f; e.speed_max = 0.3f;
    e.life_min = 0.3f;  e.life_max = 0.6f;
    e.size_start = 0.15f; e.size_end = 0.02f;
    e.color_start[0] = 255; e.color_start[1] = 255; e.color_start[2] = 255; e.color_start[3] = 180;
    e.color_end[0] = 255; e.color_end[1] = 255; e.color_end[2] = 255; e.color_end[3] = 0;
    e.gravity = 0.0f;
    e.emit_rate = 30;
    e.texture = NULL;
    return e;
}
