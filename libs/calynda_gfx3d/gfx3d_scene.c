/*
 * gfx3d_scene.c — Scene graph implementation.
 */

#include "gfx3d_scene.h"
#include "gfx3d_mesh.h"
#include "gfx3d_camera.h"

#include <stdlib.h>
#include <string.h>

/* ================================================================== */
/* Node lifecycle                                                       */
/* ================================================================== */

Gfx3dSceneNode *gfx3d_node_create(const char *name) {
    Gfx3dSceneNode *node = calloc(1, sizeof(Gfx3dSceneNode));
    if (!node) return NULL;

    node->position = cm_vec3_zero();
    node->rotation = cm_quat_identity();
    node->scale = cm_vec3_one();
    node->world_matrix = cm_mat4_identity();
    node->visible = true;
    node->has_material = false;

    if (name) {
        strncpy(node->name, name, sizeof(node->name) - 1);
        node->name[sizeof(node->name) - 1] = '\0';
    }
    return node;
}

void gfx3d_node_destroy(Gfx3dSceneNode *node) {
    if (!node) return;
    for (int i = 0; i < node->child_count; i++) {
        gfx3d_node_destroy(node->children[i]);
    }
    /* Don't free mesh/texture - they may be shared */
    free(node);
}

bool gfx3d_node_add_child(Gfx3dSceneNode *parent, Gfx3dSceneNode *child) {
    if (!parent || !child) return false;
    if (parent->child_count >= GFX3D_SCENE_MAX_CHILDREN) return false;
    child->parent = parent;
    parent->children[parent->child_count++] = child;
    return true;
}

bool gfx3d_node_remove_child(Gfx3dSceneNode *parent, Gfx3dSceneNode *child) {
    if (!parent || !child) return false;
    for (int i = 0; i < parent->child_count; i++) {
        if (parent->children[i] == child) {
            child->parent = NULL;
            parent->children[i] = parent->children[--parent->child_count];
            parent->children[parent->child_count] = NULL;
            return true;
        }
    }
    return false;
}

void gfx3d_node_set_mesh(Gfx3dSceneNode *node, Gfx3dMesh *mesh, Gfx3dMaterial mat) {
    node->mesh = mesh;
    node->material = mat;
    node->has_material = true;
}

CmMat4 gfx3d_node_local_matrix(const Gfx3dSceneNode *node) {
    return cm_mat4_trs(node->position, node->rotation, node->scale);
}

/* ================================================================== */
/* Transform update                                                     */
/* ================================================================== */

void gfx3d_node_update_transforms(Gfx3dSceneNode *node, CmMat4 parent_world) {
    CmMat4 local = gfx3d_node_local_matrix(node);
    node->world_matrix = cm_mat4_mul(parent_world, local);

    /* Update world-space bounding box */
    if (node->mesh) {
        CmAABB lb = node->mesh->bounds;
        /* Transform all 8 AABB corners and recompute bounds */
        CmVec3 corners[8] = {
            cm_vec3(lb.min.x, lb.min.y, lb.min.z),
            cm_vec3(lb.max.x, lb.min.y, lb.min.z),
            cm_vec3(lb.min.x, lb.max.y, lb.min.z),
            cm_vec3(lb.max.x, lb.max.y, lb.min.z),
            cm_vec3(lb.min.x, lb.min.y, lb.max.z),
            cm_vec3(lb.max.x, lb.min.y, lb.max.z),
            cm_vec3(lb.min.x, lb.max.y, lb.max.z),
            cm_vec3(lb.max.x, lb.max.y, lb.max.z),
        };
        CmVec3 tc = cm_mat4_mul_point(node->world_matrix, corners[0]);
        CmAABB wb = { tc, tc };
        for (int i = 1; i < 8; i++) {
            CmVec3 p = cm_mat4_mul_point(node->world_matrix, corners[i]);
            wb.min = cm_vec3_min(wb.min, p);
            wb.max = cm_vec3_max(wb.max, p);
        }
        node->world_bounds = wb;
    }

    for (int i = 0; i < node->child_count; i++) {
        gfx3d_node_update_transforms(node->children[i], node->world_matrix);
    }
}

/* ================================================================== */
/* Frustum culling                                                      */
/* ================================================================== */

void gfx3d_node_cull(Gfx3dSceneNode *node, const CmFrustum *frustum) {
    if (node->mesh) {
        node->visible = cm_frustum_contains_aabb(frustum, node->world_bounds);
    } else {
        node->visible = true; /* transform-only nodes are always "visible" */
    }

    /* Cull children regardless (they may be in frustum even if parent isn't) */
    for (int i = 0; i < node->child_count; i++) {
        gfx3d_node_cull(node->children[i], frustum);
    }
}

/* ================================================================== */
/* Rendering                                                            */
/* ================================================================== */

/* Sorting buffer for transparent objects */
typedef struct {
    Gfx3dSceneNode *node;
    float           dist_sq;
} Gfx3dRenderEntry;

#define MAX_TRANSPARENT 256
static Gfx3dRenderEntry transparent_queue[MAX_TRANSPARENT];
static int transparent_count = 0;

static bool is_transparent(const Gfx3dMaterial *mat) {
    return mat->mode == GFX3D_MAT_TRANSPARENT || mat->mode == GFX3D_MAT_ADDITIVE;
}

static void collect_and_render_opaque(Gfx3dSceneNode *node, CmVec3 camera_pos) {
    if (!node->visible) return;

    if (node->mesh && node->has_material) {
        if (is_transparent(&node->material)) {
            /* Queue transparent for later, sorted render */
            if (transparent_count < MAX_TRANSPARENT) {
                CmVec3 center = cm_aabb_center(node->world_bounds);
                transparent_queue[transparent_count].node = node;
                transparent_queue[transparent_count].dist_sq = cm_vec3_length_sq(
                    cm_vec3_sub(center, camera_pos));
                transparent_count++;
            }
        } else {
            /* Draw opaque immediately */
            gfx3d_material_apply(&node->material);
            CmMat4 view = gfx3d_camera_active_view();
            CmMat4 modelview = cm_mat4_mul(view, node->world_matrix);
            gfx3d_load_model_matrix(modelview);
            gfx3d_mesh_draw(node->mesh);
        }
    }

    for (int i = 0; i < node->child_count; i++) {
        collect_and_render_opaque(node->children[i], camera_pos);
    }
}

/* qsort comparator: back-to-front (furthest first) */
static int cmp_transparent_btf(const void *a, const void *b) {
    float da = ((const Gfx3dRenderEntry *)a)->dist_sq;
    float db = ((const Gfx3dRenderEntry *)b)->dist_sq;
    if (db > da) return 1;
    if (db < da) return -1;
    return 0;
}

void gfx3d_node_render(Gfx3dSceneNode *node, CmVec3 camera_pos) {
    transparent_count = 0;

    /* Pass 1: render opaque, collect transparent */
    collect_and_render_opaque(node, camera_pos);

    /* Pass 2: sort transparent back-to-front, then render */
    if (transparent_count > 0) {
        qsort(transparent_queue, transparent_count, sizeof(Gfx3dRenderEntry),
              cmp_transparent_btf);
        for (int i = 0; i < transparent_count; i++) {
            Gfx3dSceneNode *tn = transparent_queue[i].node;
            gfx3d_material_apply(&tn->material);
            CmMat4 view = gfx3d_camera_active_view();
            CmMat4 modelview = cm_mat4_mul(view, tn->world_matrix);
            gfx3d_load_model_matrix(modelview);
            gfx3d_mesh_draw(tn->mesh);
        }
    }
}

/* ================================================================== */
/* Search                                                               */
/* ================================================================== */

Gfx3dSceneNode *gfx3d_node_find(Gfx3dSceneNode *root, const char *name) {
    if (!root || !name) return NULL;
    if (strcmp(root->name, name) == 0) return root;
    for (int i = 0; i < root->child_count; i++) {
        Gfx3dSceneNode *found = gfx3d_node_find(root->children[i], name);
        if (found) return found;
    }
    return NULL;
}
