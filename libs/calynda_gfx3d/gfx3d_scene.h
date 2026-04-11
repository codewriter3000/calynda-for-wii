/*
 * gfx3d_scene.h — Scene graph with node tree, recursive rendering,
 * frustum culling, and depth sorting for transparent objects.
 */

#ifndef GFX3D_SCENE_H
#define GFX3D_SCENE_H

#include "../calynda_math/calynda_math.h"
#include "gfx3d_mesh.h"
#include "gfx3d_material.h"

#include <stdbool.h>
#include <stdint.h>

#define GFX3D_SCENE_MAX_CHILDREN 32

typedef struct Gfx3dSceneNode Gfx3dSceneNode;

struct Gfx3dSceneNode {
    /* Local transform */
    CmVec3  position;
    CmQuat  rotation;
    CmVec3  scale;

    /* Visual data (NULL = transform-only node) */
    Gfx3dMesh      *mesh;
    Gfx3dMaterial   material;
    bool            has_material;

    /* Hierarchy */
    Gfx3dSceneNode *parent;
    Gfx3dSceneNode *children[GFX3D_SCENE_MAX_CHILDREN];
    int             child_count;

    /* Cached world-space data */
    CmMat4  world_matrix;
    CmAABB  world_bounds;
    bool    visible;       /* set by frustum culling */

    /* User data */
    void   *userdata;
    char    name[32];
};

/* Create a new scene node */
Gfx3dSceneNode *gfx3d_node_create(const char *name);

/* Destroy a node and all its children recursively */
void gfx3d_node_destroy(Gfx3dSceneNode *node);

/* Add a child to a parent node. Returns false if full. */
bool gfx3d_node_add_child(Gfx3dSceneNode *parent, Gfx3dSceneNode *child);

/* Remove a child from its parent. Returns false if not found. */
bool gfx3d_node_remove_child(Gfx3dSceneNode *parent, Gfx3dSceneNode *child);

/* Attach a mesh + material to a node */
void gfx3d_node_set_mesh(Gfx3dSceneNode *node, Gfx3dMesh *mesh, Gfx3dMaterial mat);

/* Compute local transform matrix */
CmMat4 gfx3d_node_local_matrix(const Gfx3dSceneNode *node);

/*
 * Update world matrices recursively from root.
 * Must be called once per frame before rendering.
 */
void gfx3d_node_update_transforms(Gfx3dSceneNode *node, CmMat4 parent_world);

/*
 * Perform frustum culling on the scene tree.
 * Sets node->visible based on world_bounds vs. frustum.
 */
void gfx3d_node_cull(Gfx3dSceneNode *node, const CmFrustum *frustum);

/*
 * Render the scene tree. Draws opaque objects front-to-back,
 * then transparent objects back-to-front.
 * camera_pos: used for depth sorting.
 */
void gfx3d_node_render(Gfx3dSceneNode *node, CmVec3 camera_pos);

/* Find a node by name in the subtree (depth-first) */
Gfx3dSceneNode *gfx3d_node_find(Gfx3dSceneNode *root, const char *name);

#endif /* GFX3D_SCENE_H */
