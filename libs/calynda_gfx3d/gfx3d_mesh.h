/*
 * gfx3d_mesh.h — Mesh definition with vertex data and GX display list support.
 */

#ifndef GFX3D_MESH_H
#define GFX3D_MESH_H

#include "../calynda_math/calynda_math.h"

#include <stdbool.h>
#include <stdint.h>

/* Per-vertex data matching GX_VTXFMT0 layout */
typedef struct {
    float pos[3];
    float nrm[3];
    float uv[2];
    uint8_t color[4];   /* RGBA */
} Gfx3dVertex;

typedef struct {
    Gfx3dVertex *vertices;
    uint32_t     vertex_count;

    uint16_t    *indices;
    uint32_t     index_count;

    CmAABB       bounds;         /* local-space bounding box */

    /* GX display list (compiled once for static geometry) */
    void        *display_list;
    uint32_t     display_list_size;
    bool         display_list_valid;
} Gfx3dMesh;

/*
 * Create a mesh from vertex/index data. Makes a copy of the data.
 * Automatically computes bounding box.
 */
Gfx3dMesh *gfx3d_mesh_create(const Gfx3dVertex *verts, uint32_t vert_count,
                               const uint16_t *indices, uint32_t idx_count);

/* Free mesh and its data (including display list if compiled) */
void gfx3d_mesh_destroy(Gfx3dMesh *mesh);

/*
 * Compile the mesh into a GX display list for fast rendering.
 * Should be called once after creation for static meshes.
 * On host builds this is a no-op.
 */
void gfx3d_mesh_compile_display_list(Gfx3dMesh *mesh);

/*
 * Draw the mesh. Uses display list if compiled, otherwise immediate mode.
 * Model matrix should already be loaded into GX before calling.
 */
void gfx3d_mesh_draw(const Gfx3dMesh *mesh);

/*
 * Load a model matrix into GX position matrix slot.
 * On host builds this is a no-op.
 */
void gfx3d_load_model_matrix(CmMat4 model);

/* ================================================================== */
/* Primitive Generators                                                 */
/* ================================================================== */

/* Unit cube centered at origin (2x2x2) */
Gfx3dMesh *gfx3d_mesh_create_cube(void);

/* Sphere with given radius and segment counts */
Gfx3dMesh *gfx3d_mesh_create_sphere(float radius, int slices, int stacks);

/* Flat plane on XZ, centered at origin */
Gfx3dMesh *gfx3d_mesh_create_plane(float width, float depth, int subdiv_x, int subdiv_z);

#endif /* GFX3D_MESH_H */
