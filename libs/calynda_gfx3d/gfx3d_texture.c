/*
 * gfx3d_texture.c — Texture loading, GX binding, and LRU cache.
 */

#include "gfx3d_texture.h"

#include <stdlib.h>
#include <string.h>

#ifdef CALYNDA_WII_BUILD
#include <gccore.h>
#include <malloc.h>
#endif

static uint32_t next_tex_id = 1;

/* ================================================================== */
/* Texture creation                                                     */
/* ================================================================== */

#ifdef CALYNDA_WII_BUILD

/*
 * Convert linear RGBA8 to GX-tiled RGBA8 format.
 * GX tiles: 4x4 blocks, each block stores AR then GB interleaved.
 */
static void *rgba8_to_gx(const uint8_t *src, uint16_t w, uint16_t h, uint32_t *out_size) {
    uint32_t size = w * h * 4;
    void *dst = memalign(32, size);
    if (!dst) return NULL;
    *out_size = size;

    uint8_t *d = (uint8_t *)dst;
    for (int y = 0; y < h; y += 4) {
        for (int x = 0; x < w; x += 4) {
            /* AR block */
            for (int by = 0; by < 4; by++) {
                for (int bx = 0; bx < 4; bx++) {
                    int sx = x + bx, sy = y + by;
                    int si = (sy * w + sx) * 4;
                    *d++ = src[si + 3]; /* A */
                    *d++ = src[si + 0]; /* R */
                }
            }
            /* GB block */
            for (int by = 0; by < 4; by++) {
                for (int bx = 0; bx < 4; bx++) {
                    int sx = x + bx, sy = y + by;
                    int si = (sy * w + sx) * 4;
                    *d++ = src[si + 1]; /* G */
                    *d++ = src[si + 2]; /* B */
                }
            }
        }
    }
    DCFlushRange(dst, size);
    return dst;
}

static uint8_t wrap_to_gx(Gfx3dTexWrap w) {
    switch (w) {
        case GFX3D_TEX_WRAP_REPEAT: return GX_REPEAT;
        case GFX3D_TEX_WRAP_MIRROR: return GX_MIRROR;
        default: return GX_CLAMP;
    }
}

static uint8_t filter_to_gx(Gfx3dTexFilter f) {
    return (f == GFX3D_TEX_FILTER_LINEAR) ? GX_LINEAR : GX_NEAR;
}

static void gfx3d_texture_init_gxobj(Gfx3dTexture *tex) {
    GXTexObj *obj = (GXTexObj *)tex->gx_texobj;
    GX_InitTexObj(obj, tex->data, tex->width, tex->height,
                  GX_TF_RGBA8, wrap_to_gx(tex->wrap_s), wrap_to_gx(tex->wrap_t),
                  GX_FALSE);
    GX_InitTexObjFilterMode(obj, filter_to_gx(tex->min_filter),
                                 filter_to_gx(tex->mag_filter));
    tex->loaded = true;
}

Gfx3dTexture *gfx3d_texture_create_rgba(uint16_t width, uint16_t height,
                                          const uint8_t *rgba_data) {
    Gfx3dTexture *tex = calloc(1, sizeof(Gfx3dTexture));
    if (!tex) return NULL;

    tex->id = next_tex_id++;
    tex->width = width;
    tex->height = height;
    tex->format = GFX3D_TEX_FMT_RGBA8;
    tex->wrap_s = GFX3D_TEX_WRAP_REPEAT;
    tex->wrap_t = GFX3D_TEX_WRAP_REPEAT;
    tex->min_filter = GFX3D_TEX_FILTER_LINEAR;
    tex->mag_filter = GFX3D_TEX_FILTER_LINEAR;

    tex->data = rgba8_to_gx(rgba_data, width, height, &tex->data_size);
    if (!tex->data) { free(tex); return NULL; }

    gfx3d_texture_init_gxobj(tex);
    return tex;
}

void gfx3d_texture_bind(const Gfx3dTexture *tex, int slot) {
    if (!tex || !tex->loaded) return;
    GX_LoadTexObj((GXTexObj *)tex->gx_texobj, slot);
}

#else /* Host stubs */

Gfx3dTexture *gfx3d_texture_create_rgba(uint16_t width, uint16_t height,
                                          const uint8_t *rgba_data) {
    Gfx3dTexture *tex = calloc(1, sizeof(Gfx3dTexture));
    if (!tex) return NULL;

    tex->id = next_tex_id++;
    tex->width = width;
    tex->height = height;
    tex->format = GFX3D_TEX_FMT_RGBA8;
    tex->wrap_s = GFX3D_TEX_WRAP_REPEAT;
    tex->wrap_t = GFX3D_TEX_WRAP_REPEAT;
    tex->min_filter = GFX3D_TEX_FILTER_LINEAR;
    tex->mag_filter = GFX3D_TEX_FILTER_LINEAR;

    uint32_t size = width * height * 4;
    tex->data = malloc(size);
    if (!tex->data) { free(tex); return NULL; }
    memcpy(tex->data, rgba_data, size);
    tex->data_size = size;
    tex->loaded = true;
    return tex;
}

void gfx3d_texture_bind(const Gfx3dTexture *tex, int slot) {
    (void)tex; (void)slot;
}

#endif /* CALYNDA_WII_BUILD */

Gfx3dTexture *gfx3d_texture_create_solid(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    uint8_t pixel[4] = { r, g, b, a };
    return gfx3d_texture_create_rgba(1, 1, pixel);
}

Gfx3dTexture *gfx3d_texture_load_png(const char *path) {
    /* TODO: Implement PNG loading via PNGU on Wii, stb_image on host */
    (void)path;
    return NULL;
}

Gfx3dTexture *gfx3d_texture_load_tpl(const char *path, uint32_t tex_index) {
    /* TODO: Implement TPL loading via libogc TPL_ functions */
    (void)path; (void)tex_index;
    return NULL;
}

void gfx3d_texture_destroy(Gfx3dTexture *tex) {
    if (!tex) return;
    free(tex->data);
    free(tex);
}

/* ================================================================== */
/* Texture Cache (LRU)                                                  */
/* ================================================================== */

typedef struct {
    char           path[256];
    Gfx3dTexture  *texture;
    uint32_t       last_used;  /* frame counter for LRU */
} Gfx3dTexCacheEntry;

static Gfx3dTexCacheEntry tex_cache[GFX3D_TEX_CACHE_MAX];
static int tex_cache_count = 0;
static uint32_t tex_cache_frame = 0;

void gfx3d_tex_cache_init(void) {
    memset(tex_cache, 0, sizeof(tex_cache));
    tex_cache_count = 0;
    tex_cache_frame = 0;
}

Gfx3dTexture *gfx3d_tex_cache_get(const char *path) {
    tex_cache_frame++;

    /* Search existing */
    for (int i = 0; i < tex_cache_count; i++) {
        if (strcmp(tex_cache[i].path, path) == 0) {
            tex_cache[i].last_used = tex_cache_frame;
            return tex_cache[i].texture;
        }
    }

    /* Load new */
    size_t len = strlen(path);
    Gfx3dTexture *tex = NULL;
    if (len > 4 && strcmp(path + len - 4, ".png") == 0) {
        tex = gfx3d_texture_load_png(path);
    } else if (len > 4 && strcmp(path + len - 4, ".tpl") == 0) {
        tex = gfx3d_texture_load_tpl(path, 0);
    }
    if (!tex) return NULL;

    /* Evict if full */
    if (tex_cache_count >= GFX3D_TEX_CACHE_MAX) {
        gfx3d_tex_cache_evict(1);
    }

    if (tex_cache_count < GFX3D_TEX_CACHE_MAX) {
        Gfx3dTexCacheEntry *e = &tex_cache[tex_cache_count++];
        strncpy(e->path, path, sizeof(e->path) - 1);
        e->path[sizeof(e->path) - 1] = '\0';
        e->texture = tex;
        e->last_used = tex_cache_frame;
    }
    return tex;
}

void gfx3d_tex_cache_evict(int count) {
    for (int n = 0; n < count && tex_cache_count > 0; n++) {
        /* Find LRU entry */
        int lru_idx = 0;
        uint32_t lru_frame = tex_cache[0].last_used;
        for (int i = 1; i < tex_cache_count; i++) {
            if (tex_cache[i].last_used < lru_frame) {
                lru_frame = tex_cache[i].last_used;
                lru_idx = i;
            }
        }
        gfx3d_texture_destroy(tex_cache[lru_idx].texture);
        /* Swap with last */
        tex_cache[lru_idx] = tex_cache[--tex_cache_count];
        memset(&tex_cache[tex_cache_count], 0, sizeof(Gfx3dTexCacheEntry));
    }
}

void gfx3d_tex_cache_clear(void) {
    for (int i = 0; i < tex_cache_count; i++) {
        gfx3d_texture_destroy(tex_cache[i].texture);
    }
    memset(tex_cache, 0, sizeof(tex_cache));
    tex_cache_count = 0;
}
