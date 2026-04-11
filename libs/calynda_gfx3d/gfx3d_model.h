/*
 * gfx3d_model.h — Custom .c3d binary model format.
 *
 * Format: header + pre-swizzled GX vertex blob + index blob + material refs.
 * No text parsing at runtime — designed for fast loading on Wii.
 */

#ifndef GFX3D_MODEL_H
#define GFX3D_MODEL_H

#include "gfx3d_mesh.h"
#include "gfx3d_material.h"

#include <stdint.h>

#define C3D_MAGIC 0x43334430  /* "C3D0" */
#define C3D_VERSION 1

/* On-disk header (packed, little-endian) */
typedef struct __attribute__((packed)) {
    uint32_t magic;
    uint32_t version;
    uint32_t vertex_count;
    uint32_t index_count;
    uint32_t vertex_data_offset;  /* offset from file start */
    uint32_t index_data_offset;
    uint32_t material_name_offset; /* offset to null-terminated material name */
    float    bounds_min[3];
    float    bounds_max[3];
} C3dHeader;

/* A loaded .c3d model with mesh + material name */
typedef struct {
    Gfx3dMesh     *mesh;
    char           material_name[64];
} Gfx3dModel;

/* Load a .c3d model from a file path */
Gfx3dModel *gfx3d_model_load_c3d(const char *path);

/* Load a .c3d model from a memory buffer */
Gfx3dModel *gfx3d_model_load_c3d_mem(const void *data, uint32_t size);

/* Free model */
void gfx3d_model_destroy(Gfx3dModel *model);

#endif /* GFX3D_MODEL_H */
