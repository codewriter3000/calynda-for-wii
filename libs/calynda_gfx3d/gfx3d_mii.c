/*
 * gfx3d_mii.c — Mii character rendering with color palettes.
 */

#include "gfx3d_mii.h"
#include "gfx3d_mesh.h"

#include <string.h>

/* ================================================================== */
/* Color Palettes (matching official Wii Mii colors)                    */
/* ================================================================== */

const uint8_t GFX3D_MII_SKIN_COLORS[6][3] = {
    { 252, 216, 168 },   /* 0: Light */
    { 240, 188, 132 },   /* 1: Natural */
    { 216, 152,  96 },   /* 2: Medium */
    { 180, 120,  68 },   /* 3: Dark */
    { 140,  88,  52 },   /* 4: Darker */
    { 100,  60,  36 },   /* 5: Darkest */
};

const uint8_t GFX3D_MII_HAIR_COLORS[8][3] = {
    {  40,  40,  40 },   /* 0: Black */
    { 100,  60,  32 },   /* 1: Brown */
    { 180, 120,  48 },   /* 2: Light Brown */
    { 220, 180,  80 },   /* 3: Blonde */
    { 180,  40,  24 },   /* 4: Red */
    { 128, 128, 128 },   /* 5: Gray */
    { 200, 200, 200 },   /* 6: White */
    { 220, 120,  48 },   /* 7: Orange */
};

const uint8_t GFX3D_MII_FAV_COLORS[12][3] = {
    { 200,  40,  40 },   /* 0: Red */
    { 240, 140,  40 },   /* 1: Orange */
    { 240, 220,  40 },   /* 2: Yellow */
    { 120, 200,  80 },   /* 3: Light Green */
    {  40, 120,  40 },   /* 4: Dark Green */
    {  40, 120, 200 },   /* 5: Blue */
    {  60, 180, 220 },   /* 6: Light Blue */
    { 220,  80, 160 },   /* 7: Pink */
    { 120,  40, 160 },   /* 8: Purple */
    { 100,  60,  40 },   /* 9: Brown */
    { 240, 240, 240 },   /* 10: White */
    {  40,  40,  40 },   /* 11: Black */
};

Gfx3dMiiModel gfx3d_mii_build(uint8_t skin_color, uint8_t hair_color,
                                uint8_t eye_type, uint8_t fav_color,
                                uint8_t height, uint8_t weight,
                                uint8_t gender) {
    Gfx3dMiiModel mii;
    memset(&mii, 0, sizeof(mii));

    mii.skin_color = skin_color < 6 ? skin_color : 0;
    mii.hair_color = hair_color < 8 ? hair_color : 0;
    mii.eye_type = eye_type;
    mii.fav_color = fav_color < 12 ? fav_color : 0;
    mii.height = height;
    mii.weight = weight;
    mii.gender = gender;

    /* Generate solid-color textures for now.
       A full implementation would use sprite sheets with eye/mouth variations. */
    const uint8_t *sc = GFX3D_MII_SKIN_COLORS[mii.skin_color];
    mii.skin_tex = gfx3d_texture_create_solid(sc[0], sc[1], sc[2], 255);

    const uint8_t *hc = GFX3D_MII_HAIR_COLORS[mii.hair_color];
    mii.hair_tex = gfx3d_texture_create_solid(hc[0], hc[1], hc[2], 255);

    /* Eye texture — placeholder white */
    mii.eye_tex = gfx3d_texture_create_solid(255, 255, 255, 255);

    /* Face uses skin with slight variation */
    mii.face_tex = gfx3d_texture_create_solid(sc[0], sc[1], sc[2], 255);

    /* Build materials */
    mii.skin_mat = gfx3d_material_textured(mii.skin_tex);
    mii.face_mat = gfx3d_material_textured(mii.face_tex);
    mii.hair_mat = gfx3d_material_textured(mii.hair_tex);

    const uint8_t *fc = GFX3D_MII_FAV_COLORS[mii.fav_color];
    Gfx3dTexture *shirt_tex = gfx3d_texture_create_solid(fc[0], fc[1], fc[2], 255);
    mii.shirt_mat = gfx3d_material_textured(shirt_tex);

    Gfx3dTexture *pants_tex = gfx3d_texture_create_solid(60, 60, 80, 255);
    mii.pants_mat = gfx3d_material_textured(pants_tex);

    /* Mesh parts are NULL — caller must assign loaded meshes */

    return mii;
}

void gfx3d_mii_draw(const Gfx3dMiiModel *mii, CmMat4 world) {
    /* Scale based on height (0-127 → 0.8-1.2 range) */
    float h_scale = 0.8f + (float)mii->height / 127.0f * 0.4f;
    /* Width based on weight */
    float w_scale = 0.85f + (float)mii->weight / 127.0f * 0.3f;

    CmMat4 body_scale = cm_mat4_scale(cm_vec3(w_scale, h_scale, w_scale));
    CmMat4 model = cm_mat4_mul(world, body_scale);

    if (mii->body) {
        gfx3d_material_apply(&mii->shirt_mat);
        gfx3d_load_model_matrix(model);
        gfx3d_mesh_draw(mii->body);
    }

    if (mii->head) {
        /* Head sits on top of body — offset up by body height */
        CmMat4 head_offset = cm_mat4_translate(cm_vec3(0, h_scale * 1.0f, 0));
        CmMat4 head_model = cm_mat4_mul(model, head_offset);
        gfx3d_material_apply(&mii->skin_mat);
        gfx3d_load_model_matrix(head_model);
        gfx3d_mesh_draw(mii->head);
    }

    if (mii->arms) {
        gfx3d_material_apply(&mii->skin_mat);
        gfx3d_load_model_matrix(model);
        gfx3d_mesh_draw(mii->arms);
    }

    if (mii->legs) {
        gfx3d_material_apply(&mii->pants_mat);
        gfx3d_load_model_matrix(model);
        gfx3d_mesh_draw(mii->legs);
    }
}

void gfx3d_mii_destroy(Gfx3dMiiModel *mii) {
    /* Free generated textures */
    gfx3d_texture_destroy(mii->skin_tex);
    gfx3d_texture_destroy(mii->hair_tex);
    gfx3d_texture_destroy(mii->eye_tex);
    gfx3d_texture_destroy(mii->face_tex);
    /* Meshes are not owned by the Mii model */
    memset(mii, 0, sizeof(Gfx3dMiiModel));
}
