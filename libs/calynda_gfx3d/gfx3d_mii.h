/*
 * gfx3d_mii.h — Map Mii face/body parameters to 3D mesh parts.
 *
 * Uses Mii data readable from Wiimote EEPROM (via calynda_mii.h)
 * to assemble a character model with correct skin/hair/eye textures.
 */

#ifndef GFX3D_MII_H
#define GFX3D_MII_H

#include "gfx3d_mesh.h"
#include "gfx3d_material.h"
#include "gfx3d_texture.h"
#include "../calynda_math/calynda_math.h"

#include <stdint.h>

/* Mii body part mesh slots */
typedef struct {
    Gfx3dMesh     *head;
    Gfx3dMesh     *body;
    Gfx3dMesh     *arms;
    Gfx3dMesh     *legs;

    Gfx3dTexture  *skin_tex;
    Gfx3dTexture  *face_tex;
    Gfx3dTexture  *hair_tex;
    Gfx3dTexture  *eye_tex;

    Gfx3dMaterial  skin_mat;
    Gfx3dMaterial  face_mat;
    Gfx3dMaterial  hair_mat;
    Gfx3dMaterial  shirt_mat;
    Gfx3dMaterial  pants_mat;

    /* Mii parameters (cached from EEPROM data) */
    uint8_t        skin_color;    /* 0-5: index into skin palette */
    uint8_t        hair_color;    /* 0-7: index into hair palette */
    uint8_t        eye_type;
    uint8_t        fav_color;     /* 0-11: shirt/accent color */
    uint8_t        height;        /* 0-127 */
    uint8_t        weight;        /* 0-127 */
    uint8_t        gender;        /* 0=male, 1=female */
} Gfx3dMiiModel;

/* Skin color palette (RGB) */
extern const uint8_t GFX3D_MII_SKIN_COLORS[6][3];

/* Hair color palette (RGB) */
extern const uint8_t GFX3D_MII_HAIR_COLORS[8][3];

/* Favorite color palette (shirt colors, RGB) */
extern const uint8_t GFX3D_MII_FAV_COLORS[12][3];

/*
 * Build a Mii 3D model from raw Mii parameters.
 * Uses pre-made base mesh parts (must be loaded separately)
 * and generates textures with correct colors.
 */
Gfx3dMiiModel gfx3d_mii_build(uint8_t skin_color, uint8_t hair_color,
                                uint8_t eye_type, uint8_t fav_color,
                                uint8_t height, uint8_t weight,
                                uint8_t gender);

/*
 * Draw the Mii model at the given world-space transform.
 */
void gfx3d_mii_draw(const Gfx3dMiiModel *mii, CmMat4 world);

/* Free Mii model resources (textures created by build) */
void gfx3d_mii_destroy(Gfx3dMiiModel *mii);

#endif /* GFX3D_MII_H */
