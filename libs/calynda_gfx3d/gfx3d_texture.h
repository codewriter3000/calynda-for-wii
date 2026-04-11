/*
 * gfx3d_texture.h — Texture loading, caching, and management.
 *
 * Supports PNG (via PNGU on Wii), TPL (native Wii format), and
 * raw RGBA buffers. Includes LRU texture cache for memory management.
 */

#ifndef GFX3D_TEXTURE_H
#define GFX3D_TEXTURE_H

#include <stdbool.h>
#include <stdint.h>

/* Texture pixel formats */
typedef enum {
    GFX3D_TEX_FMT_RGBA8,
    GFX3D_TEX_FMT_RGB565,
    GFX3D_TEX_FMT_IA8,
    GFX3D_TEX_FMT_I8,
    GFX3D_TEX_FMT_CMPR       /* S3TC / DXT1 compressed */
} Gfx3dTexFormat;

/* Texture wrap modes */
typedef enum {
    GFX3D_TEX_WRAP_CLAMP,
    GFX3D_TEX_WRAP_REPEAT,
    GFX3D_TEX_WRAP_MIRROR
} Gfx3dTexWrap;

/* Texture filter modes */
typedef enum {
    GFX3D_TEX_FILTER_NEAREST,
    GFX3D_TEX_FILTER_LINEAR
} Gfx3dTexFilter;

typedef struct {
    uint32_t      id;            /* unique texture ID */
    uint16_t      width;
    uint16_t      height;
    Gfx3dTexFormat format;
    void         *data;          /* texture data (GX-tiled on Wii) */
    uint32_t      data_size;
    Gfx3dTexWrap  wrap_s;
    Gfx3dTexWrap  wrap_t;
    Gfx3dTexFilter min_filter;
    Gfx3dTexFilter mag_filter;
    bool          loaded;        /* true if bound to GX tex object */

#ifdef CALYNDA_WII_BUILD
    /* Opaque GX texture object storage */
    uint8_t       gx_texobj[64]; /* sizeof(GXTexObj) */
#endif
} Gfx3dTexture;

/* Create a texture from raw RGBA8 pixel data */
Gfx3dTexture *gfx3d_texture_create_rgba(uint16_t width, uint16_t height,
                                          const uint8_t *rgba_data);

/* Load a PNG texture from a file path (uses PNGU on Wii) */
Gfx3dTexture *gfx3d_texture_load_png(const char *path);

/* Load a TPL texture from a file path (Wii-native format) */
Gfx3dTexture *gfx3d_texture_load_tpl(const char *path, uint32_t tex_index);

/* Generate a 1x1 solid color texture (useful for untextured materials) */
Gfx3dTexture *gfx3d_texture_create_solid(uint8_t r, uint8_t g, uint8_t b, uint8_t a);

/* Bind texture per-primitive for upcoming draw calls */
void gfx3d_texture_bind(const Gfx3dTexture *tex, int slot);

/* Free a texture and its GPU data */
void gfx3d_texture_destroy(Gfx3dTexture *tex);

/* ================================================================== */
/* Texture Cache (LRU)                                                  */
/* ================================================================== */

#define GFX3D_TEX_CACHE_MAX 64

/* Initialize the texture cache */
void gfx3d_tex_cache_init(void);

/* Get-or-load a texture by path (returns cached if available) */
Gfx3dTexture *gfx3d_tex_cache_get(const char *path);

/* Evict least-recently-used textures to free memory */
void gfx3d_tex_cache_evict(int count);

/* Clear entire cache */
void gfx3d_tex_cache_clear(void);

#endif /* GFX3D_TEXTURE_H */
