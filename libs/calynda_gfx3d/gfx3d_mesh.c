/*
 * gfx3d_mesh.c — Mesh creation, drawing, display list compilation,
 * and primitive generators.
 */

#include "gfx3d_mesh.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#ifdef CALYNDA_WII_BUILD
#include <gccore.h>
#include <malloc.h>
#endif

/* ================================================================== */
/* Mesh lifecycle                                                       */
/* ================================================================== */

static CmAABB compute_bounds(const Gfx3dVertex *verts, uint32_t count) {
    CmAABB b;
    if (count == 0) {
        b.min = b.max = cm_vec3_zero();
        return b;
    }
    b.min.x = b.max.x = verts[0].pos[0];
    b.min.y = b.max.y = verts[0].pos[1];
    b.min.z = b.max.z = verts[0].pos[2];
    for (uint32_t i = 1; i < count; i++) {
        if (verts[i].pos[0] < b.min.x) b.min.x = verts[i].pos[0];
        if (verts[i].pos[1] < b.min.y) b.min.y = verts[i].pos[1];
        if (verts[i].pos[2] < b.min.z) b.min.z = verts[i].pos[2];
        if (verts[i].pos[0] > b.max.x) b.max.x = verts[i].pos[0];
        if (verts[i].pos[1] > b.max.y) b.max.y = verts[i].pos[1];
        if (verts[i].pos[2] > b.max.z) b.max.z = verts[i].pos[2];
    }
    return b;
}

Gfx3dMesh *gfx3d_mesh_create(const Gfx3dVertex *verts, uint32_t vert_count,
                               const uint16_t *indices, uint32_t idx_count) {
    Gfx3dMesh *mesh = calloc(1, sizeof(Gfx3dMesh));
    if (!mesh) return NULL;

    mesh->vertex_count = vert_count;
    mesh->vertices = malloc(sizeof(Gfx3dVertex) * vert_count);
    if (!mesh->vertices) { free(mesh); return NULL; }
    memcpy(mesh->vertices, verts, sizeof(Gfx3dVertex) * vert_count);

    mesh->index_count = idx_count;
    mesh->indices = malloc(sizeof(uint16_t) * idx_count);
    if (!mesh->indices) { free(mesh->vertices); free(mesh); return NULL; }
    memcpy(mesh->indices, indices, sizeof(uint16_t) * idx_count);

    mesh->bounds = compute_bounds(verts, vert_count);
    mesh->display_list = NULL;
    mesh->display_list_size = 0;
    mesh->display_list_valid = false;

    return mesh;
}

void gfx3d_mesh_destroy(Gfx3dMesh *mesh) {
    if (!mesh) return;
    free(mesh->vertices);
    free(mesh->indices);
    if (mesh->display_list) {
#ifdef CALYNDA_WII_BUILD
        free(MEM_PHYSICAL_TO_K0(mesh->display_list));
#else
        free(mesh->display_list);
#endif
    }
    free(mesh);
}

/* ================================================================== */
/* Drawing                                                              */
/* ================================================================== */

#ifdef CALYNDA_WII_BUILD

static void gfx3d_mesh_draw_immediate(const Gfx3dMesh *mesh) {
    GX_Begin(GX_TRIANGLES, GX_VTXFMT0, mesh->index_count);
    for (uint32_t i = 0; i < mesh->index_count; i++) {
        const Gfx3dVertex *v = &mesh->vertices[mesh->indices[i]];
        GX_Position3f32(v->pos[0], v->pos[1], v->pos[2]);
        GX_Normal3f32(v->nrm[0], v->nrm[1], v->nrm[2]);
        GX_Color4u8(v->color[0], v->color[1], v->color[2], v->color[3]);
        GX_TexCoord2f32(v->uv[0], v->uv[1]);
    }
    GX_End();
}

void gfx3d_mesh_compile_display_list(Gfx3dMesh *mesh) {
    if (mesh->display_list_valid) return;

    /* Estimate size: each vertex submit is ~(12+12+8+4)=36 bytes,
       plus GX overhead. Allocate generously, aligned to 32 bytes. */
    uint32_t est_size = (mesh->index_count * 48) + 256;
    est_size = (est_size + 31) & ~31u;

    void *dl = memalign(32, est_size);
    if (!dl) return;
    memset(dl, 0, est_size);
    DCInvalidateRange(dl, est_size);

    GX_BeginDispList(dl, est_size);

    GX_Begin(GX_TRIANGLES, GX_VTXFMT0, mesh->index_count);
    for (uint32_t i = 0; i < mesh->index_count; i++) {
        const Gfx3dVertex *v = &mesh->vertices[mesh->indices[i]];
        GX_Position3f32(v->pos[0], v->pos[1], v->pos[2]);
        GX_Normal3f32(v->nrm[0], v->nrm[1], v->nrm[2]);
        GX_Color4u8(v->color[0], v->color[1], v->color[2], v->color[3]);
        GX_TexCoord2f32(v->uv[0], v->uv[1]);
    }
    GX_End();

    uint32_t actual_size = GX_EndDispList();
    if (actual_size == 0) {
        free(dl);
        return;
    }

    mesh->display_list = dl;
    mesh->display_list_size = actual_size;
    mesh->display_list_valid = true;
}

void gfx3d_mesh_draw(const Gfx3dMesh *mesh) {
    if (mesh->display_list_valid) {
        GX_CallDispList(mesh->display_list, mesh->display_list_size);
    } else {
        gfx3d_mesh_draw_immediate(mesh);
    }
}

void gfx3d_load_model_matrix(CmMat4 model) {
    Mtx mtx;
    /* CmMat4 is column-major, libogc Mtx is row-major 3x4 */
    for (int row = 0; row < 3; row++)
        for (int col = 0; col < 4; col++)
            mtx[row][col] = model.m[col * 4 + row];
    GX_LoadPosMtxImm(mtx, GX_PNMTX0);

    /* Normal matrix: inverse-transpose of upper 3x3 */
    Mtx nrm;
    guMtxInverse(mtx, nrm);
    guMtxTranspose(nrm, nrm);
    GX_LoadNrmMtxImm(nrm, GX_PNMTX0);
}

#else /* Host stubs */

void gfx3d_mesh_compile_display_list(Gfx3dMesh *mesh) {
    mesh->display_list_valid = false; /* no-op on host */
}

void gfx3d_mesh_draw(const Gfx3dMesh *mesh) {
    (void)mesh; /* no-op on host */
}

void gfx3d_load_model_matrix(CmMat4 model) {
    (void)model; /* no-op on host */
}

#endif /* CALYNDA_WII_BUILD */

/* ================================================================== */
/* Primitive Generators                                                 */
/* ================================================================== */

static void set_vertex(Gfx3dVertex *v, float px, float py, float pz,
                        float nx, float ny, float nz,
                        float u, float vt,
                        uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    v->pos[0] = px; v->pos[1] = py; v->pos[2] = pz;
    v->nrm[0] = nx; v->nrm[1] = ny; v->nrm[2] = nz;
    v->uv[0] = u;   v->uv[1] = vt;
    v->color[0] = r; v->color[1] = g; v->color[2] = b; v->color[3] = a;
}

Gfx3dMesh *gfx3d_mesh_create_cube(void) {
    /* 24 vertices (4 per face for correct normals), 36 indices */
    Gfx3dVertex verts[24];
    uint16_t indices[36];
    int vi = 0, ii = 0;

    /* Face definitions: normal, then 4 corners (CCW) */
    const float faces[6][4][3] = {
        /* +Z */ {{ -1,-1, 1}, { 1,-1, 1}, { 1, 1, 1}, {-1, 1, 1}},
        /* -Z */ {{  1,-1,-1}, {-1,-1,-1}, {-1, 1,-1}, { 1, 1,-1}},
        /* +Y */ {{ -1, 1, 1}, { 1, 1, 1}, { 1, 1,-1}, {-1, 1,-1}},
        /* -Y */ {{ -1,-1,-1}, { 1,-1,-1}, { 1,-1, 1}, {-1,-1, 1}},
        /* +X */ {{  1,-1, 1}, { 1,-1,-1}, { 1, 1,-1}, { 1, 1, 1}},
        /* -X */ {{ -1,-1,-1}, {-1,-1, 1}, {-1, 1, 1}, {-1, 1,-1}},
    };
    const float normals[6][3] = {
        {0,0,1}, {0,0,-1}, {0,1,0}, {0,-1,0}, {1,0,0}, {-1,0,0}
    };
    const float uvs[4][2] = {{0,1},{1,1},{1,0},{0,0}};

    for (int f = 0; f < 6; f++) {
        int base = vi;
        for (int c = 0; c < 4; c++) {
            set_vertex(&verts[vi],
                       faces[f][c][0], faces[f][c][1], faces[f][c][2],
                       normals[f][0], normals[f][1], normals[f][2],
                       uvs[c][0], uvs[c][1],
                       255, 255, 255, 255);
            vi++;
        }
        indices[ii++] = base;     indices[ii++] = base + 1; indices[ii++] = base + 2;
        indices[ii++] = base;     indices[ii++] = base + 2; indices[ii++] = base + 3;
    }

    return gfx3d_mesh_create(verts, 24, indices, 36);
}

Gfx3dMesh *gfx3d_mesh_create_sphere(float radius, int slices, int stacks) {
    int vert_count = (slices + 1) * (stacks + 1);
    int idx_count = slices * stacks * 6;
    Gfx3dVertex *verts = malloc(sizeof(Gfx3dVertex) * vert_count);
    uint16_t *indices = malloc(sizeof(uint16_t) * idx_count);
    if (!verts || !indices) { free(verts); free(indices); return NULL; }

    int vi = 0;
    for (int j = 0; j <= stacks; j++) {
        float phi = CM_PI * (float)j / (float)stacks;
        float sp = sinf(phi), cp = cosf(phi);
        for (int i = 0; i <= slices; i++) {
            float theta = 2.0f * CM_PI * (float)i / (float)slices;
            float st = sinf(theta), ct = cosf(theta);

            float nx = sp * ct, ny = cp, nz = sp * st;
            set_vertex(&verts[vi],
                       nx * radius, ny * radius, nz * radius,
                       nx, ny, nz,
                       (float)i / (float)slices, (float)j / (float)stacks,
                       255, 255, 255, 255);
            vi++;
        }
    }

    int ii = 0;
    for (int j = 0; j < stacks; j++) {
        for (int i = 0; i < slices; i++) {
            int a = j * (slices + 1) + i;
            int b = a + slices + 1;
            indices[ii++] = a;     indices[ii++] = b;     indices[ii++] = a + 1;
            indices[ii++] = a + 1; indices[ii++] = b;     indices[ii++] = b + 1;
        }
    }

    Gfx3dMesh *mesh = gfx3d_mesh_create(verts, vert_count, indices, idx_count);
    free(verts);
    free(indices);
    return mesh;
}

Gfx3dMesh *gfx3d_mesh_create_plane(float width, float depth, int subdiv_x, int subdiv_z) {
    int cols = subdiv_x + 1;
    int rows = subdiv_z + 1;
    int vert_count = cols * rows;
    int idx_count = subdiv_x * subdiv_z * 6;
    Gfx3dVertex *verts = malloc(sizeof(Gfx3dVertex) * vert_count);
    uint16_t *indices = malloc(sizeof(uint16_t) * idx_count);
    if (!verts || !indices) { free(verts); free(indices); return NULL; }

    int vi = 0;
    for (int z = 0; z < rows; z++) {
        for (int x = 0; x < cols; x++) {
            float px = -width * 0.5f + width * (float)x / (float)subdiv_x;
            float pz = -depth * 0.5f + depth * (float)z / (float)subdiv_z;
            set_vertex(&verts[vi],
                       px, 0.0f, pz,
                       0, 1, 0,
                       (float)x / (float)subdiv_x, (float)z / (float)subdiv_z,
                       255, 255, 255, 255);
            vi++;
        }
    }

    int ii = 0;
    for (int z = 0; z < subdiv_z; z++) {
        for (int x = 0; x < subdiv_x; x++) {
            int a = z * cols + x;
            int b = a + cols;
            indices[ii++] = a;     indices[ii++] = b;     indices[ii++] = a + 1;
            indices[ii++] = a + 1; indices[ii++] = b;     indices[ii++] = b + 1;
        }
    }

    Gfx3dMesh *mesh = gfx3d_mesh_create(verts, vert_count, indices, idx_count);
    free(verts);
    free(indices);
    return mesh;
}
