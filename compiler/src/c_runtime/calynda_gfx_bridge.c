/*
 * calynda_gfx_bridge.c — Graphics bridge: import game.gfx;
 *
 * Callable kind range: 300–399
 * All functions operate via CalyndaRtWord arguments.
 * Float arguments are passed as reinterpreted-cast words.
 *
 * On Wii builds (-DCALYNDA_WII_BUILD), dispatches to the real
 * calynda_gfx3d library.  On host, prints diagnostic stubs.
 */

#include "calynda_gfx_bridge.h"

#include <stdio.h>
#include <string.h>

#ifdef CALYNDA_WII_BUILD
#include "gfx3d.h"
#endif

/* ------------------------------------------------------------------ */
/* Float ↔ Word conversion                                              */
/* ------------------------------------------------------------------ */

static inline float word_to_float(CalyndaRtWord w) {
    union { CalyndaRtWord w; float f; } u;
    u.w = w;
    return u.f;
}

static inline CalyndaRtWord float_to_word(float f) {
    union { float f; CalyndaRtWord w; } u;
    u.w = 0;
    u.f = f;
    return u.w;
}

/* ------------------------------------------------------------------ */
/* Package                                                              */
/* ------------------------------------------------------------------ */

CalyndaRtPackage __calynda_pkg_gfx = {
    { CALYNDA_RT_OBJECT_MAGIC, CALYNDA_RT_OBJECT_PACKAGE },
    "gfx"
};

/* ------------------------------------------------------------------ */
/* Callable IDs                                                         */
/* ------------------------------------------------------------------ */

enum {
    GFX_CALL_INIT = 300,
    GFX_CALL_FRAME_BEGIN,
    GFX_CALL_FRAME_END,
    GFX_CALL_SETUP_3D,
    GFX_CALL_SETUP_2D,

    /* Camera */
    GFX_CALL_CAMERA_SET_POS,
    GFX_CALL_CAMERA_SET_TARGET,
    GFX_CALL_CAMERA_SET_FOV,
    GFX_CALL_CAMERA_APPLY,
    GFX_CALL_CAMERA_ORBIT_UPDATE,
    GFX_CALL_CAMERA_FOLLOW_UPDATE,

    /* Mesh */
    GFX_CALL_MESH_CREATE_CUBE,
    GFX_CALL_MESH_CREATE_SPHERE,
    GFX_CALL_MESH_CREATE_PLANE,
    GFX_CALL_MESH_DRAW,

    /* Material */
    GFX_CALL_MATERIAL_SET_MODE,
    GFX_CALL_MATERIAL_APPLY,

    /* Lights */
    GFX_CALL_LIGHT_SET_AMBIENT,
    GFX_CALL_LIGHT_ADD_DIRECTIONAL,
    GFX_CALL_LIGHT_ADD_POINT,
    GFX_CALL_LIGHTS_APPLY,

    /* Scene */
    GFX_CALL_SCENE_CREATE_NODE,
    GFX_CALL_NODE_SET_POS,
    GFX_CALL_NODE_SET_ROTATION,
    GFX_CALL_NODE_SET_SCALE,
    GFX_CALL_NODE_SET_MESH,
    GFX_CALL_SCENE_RENDER,

    /* Skybox */
    GFX_CALL_SKYBOX_DRAW,

    /* Particles */
    GFX_CALL_PARTICLES_EMIT,
    GFX_CALL_PARTICLES_UPDATE,
    GFX_CALL_PARTICLES_DRAW,

    /* Water */
    GFX_CALL_WATER_UPDATE,
    GFX_CALL_WATER_DRAW,
};

/* ------------------------------------------------------------------ */
/* Callable Objects                                                     */
/* ------------------------------------------------------------------ */

#define GFX_CALLABLE(var, id, label) \
    static CalyndaRtExternCallable var = { \
        { CALYNDA_RT_OBJECT_MAGIC, CALYNDA_RT_OBJECT_EXTERN_CALLABLE }, \
        (CalyndaRtExternCallableKind)(id), \
        label \
    }

GFX_CALLABLE(GFX_INIT_CALLABLE,             GFX_CALL_INIT,              "init");
GFX_CALLABLE(GFX_FRAME_BEGIN_CALLABLE,       GFX_CALL_FRAME_BEGIN,       "frame_begin");
GFX_CALLABLE(GFX_FRAME_END_CALLABLE,         GFX_CALL_FRAME_END,         "frame_end");
GFX_CALLABLE(GFX_SETUP_3D_CALLABLE,          GFX_CALL_SETUP_3D,          "setup_3d");
GFX_CALLABLE(GFX_SETUP_2D_CALLABLE,          GFX_CALL_SETUP_2D,          "setup_2d");
GFX_CALLABLE(GFX_CAMERA_SET_POS_CALLABLE,    GFX_CALL_CAMERA_SET_POS,    "camera_set_pos");
GFX_CALLABLE(GFX_CAMERA_SET_TARGET_CALLABLE, GFX_CALL_CAMERA_SET_TARGET, "camera_set_target");
GFX_CALLABLE(GFX_CAMERA_SET_FOV_CALLABLE,    GFX_CALL_CAMERA_SET_FOV,    "camera_set_fov");
GFX_CALLABLE(GFX_CAMERA_APPLY_CALLABLE,      GFX_CALL_CAMERA_APPLY,      "camera_apply");
GFX_CALLABLE(GFX_CAMERA_ORBIT_CALLABLE,      GFX_CALL_CAMERA_ORBIT_UPDATE, "camera_orbit_update");
GFX_CALLABLE(GFX_CAMERA_FOLLOW_CALLABLE,     GFX_CALL_CAMERA_FOLLOW_UPDATE, "camera_follow_update");
GFX_CALLABLE(GFX_MESH_CUBE_CALLABLE,         GFX_CALL_MESH_CREATE_CUBE,  "mesh_create_cube");
GFX_CALLABLE(GFX_MESH_SPHERE_CALLABLE,       GFX_CALL_MESH_CREATE_SPHERE,"mesh_create_sphere");
GFX_CALLABLE(GFX_MESH_PLANE_CALLABLE,        GFX_CALL_MESH_CREATE_PLANE, "mesh_create_plane");
GFX_CALLABLE(GFX_MESH_DRAW_CALLABLE,         GFX_CALL_MESH_DRAW,         "mesh_draw");
GFX_CALLABLE(GFX_MAT_MODE_CALLABLE,          GFX_CALL_MATERIAL_SET_MODE, "material_set_mode");
GFX_CALLABLE(GFX_MAT_APPLY_CALLABLE,         GFX_CALL_MATERIAL_APPLY,    "material_apply");
GFX_CALLABLE(GFX_LIGHT_AMBIENT_CALLABLE,     GFX_CALL_LIGHT_SET_AMBIENT, "light_set_ambient");
GFX_CALLABLE(GFX_LIGHT_DIR_CALLABLE,         GFX_CALL_LIGHT_ADD_DIRECTIONAL, "light_add_directional");
GFX_CALLABLE(GFX_LIGHT_POINT_CALLABLE,       GFX_CALL_LIGHT_ADD_POINT,   "light_add_point");
GFX_CALLABLE(GFX_LIGHTS_APPLY_CALLABLE,      GFX_CALL_LIGHTS_APPLY,      "lights_apply");
GFX_CALLABLE(GFX_SCENE_NODE_CALLABLE,        GFX_CALL_SCENE_CREATE_NODE, "scene_create_node");
GFX_CALLABLE(GFX_NODE_SET_POS_CALLABLE,      GFX_CALL_NODE_SET_POS,      "node_set_pos");
GFX_CALLABLE(GFX_NODE_SET_ROT_CALLABLE,      GFX_CALL_NODE_SET_ROTATION, "node_set_rotation");
GFX_CALLABLE(GFX_NODE_SET_SCALE_CALLABLE,    GFX_CALL_NODE_SET_SCALE,    "node_set_scale");
GFX_CALLABLE(GFX_NODE_SET_MESH_CALLABLE,     GFX_CALL_NODE_SET_MESH,     "node_set_mesh");
GFX_CALLABLE(GFX_SCENE_RENDER_CALLABLE,      GFX_CALL_SCENE_RENDER,      "scene_render");
GFX_CALLABLE(GFX_SKYBOX_DRAW_CALLABLE,       GFX_CALL_SKYBOX_DRAW,       "skybox_draw");
GFX_CALLABLE(GFX_PARTICLES_EMIT_CALLABLE,    GFX_CALL_PARTICLES_EMIT,    "particles_emit");
GFX_CALLABLE(GFX_PARTICLES_UPDATE_CALLABLE,  GFX_CALL_PARTICLES_UPDATE,  "particles_update");
GFX_CALLABLE(GFX_PARTICLES_DRAW_CALLABLE,    GFX_CALL_PARTICLES_DRAW,    "particles_draw");
GFX_CALLABLE(GFX_WATER_UPDATE_CALLABLE,      GFX_CALL_WATER_UPDATE,      "water_update");
GFX_CALLABLE(GFX_WATER_DRAW_CALLABLE,        GFX_CALL_WATER_DRAW,        "water_draw");

#undef GFX_CALLABLE

static CalyndaRtExternCallable *ALL_GFX_CALLABLES[] = {
    &GFX_INIT_CALLABLE, &GFX_FRAME_BEGIN_CALLABLE, &GFX_FRAME_END_CALLABLE,
    &GFX_SETUP_3D_CALLABLE, &GFX_SETUP_2D_CALLABLE,
    &GFX_CAMERA_SET_POS_CALLABLE, &GFX_CAMERA_SET_TARGET_CALLABLE,
    &GFX_CAMERA_SET_FOV_CALLABLE, &GFX_CAMERA_APPLY_CALLABLE,
    &GFX_CAMERA_ORBIT_CALLABLE, &GFX_CAMERA_FOLLOW_CALLABLE,
    &GFX_MESH_CUBE_CALLABLE, &GFX_MESH_SPHERE_CALLABLE,
    &GFX_MESH_PLANE_CALLABLE, &GFX_MESH_DRAW_CALLABLE,
    &GFX_MAT_MODE_CALLABLE, &GFX_MAT_APPLY_CALLABLE,
    &GFX_LIGHT_AMBIENT_CALLABLE, &GFX_LIGHT_DIR_CALLABLE,
    &GFX_LIGHT_POINT_CALLABLE, &GFX_LIGHTS_APPLY_CALLABLE,
    &GFX_SCENE_NODE_CALLABLE, &GFX_NODE_SET_POS_CALLABLE,
    &GFX_NODE_SET_ROT_CALLABLE, &GFX_NODE_SET_SCALE_CALLABLE,
    &GFX_NODE_SET_MESH_CALLABLE, &GFX_SCENE_RENDER_CALLABLE, &GFX_SKYBOX_DRAW_CALLABLE,
    &GFX_PARTICLES_EMIT_CALLABLE, &GFX_PARTICLES_UPDATE_CALLABLE,
    &GFX_PARTICLES_DRAW_CALLABLE, &GFX_WATER_UPDATE_CALLABLE,
    &GFX_WATER_DRAW_CALLABLE,
};
#define GFX_CALLABLE_COUNT (sizeof(ALL_GFX_CALLABLES) / sizeof(ALL_GFX_CALLABLES[0]))

/* ------------------------------------------------------------------ */
/* Registration                                                         */
/* ------------------------------------------------------------------ */

void calynda_gfx_register_objects(void) {
    calynda_rt_register_static_object(&__calynda_pkg_gfx.header);
    for (size_t i = 0; i < GFX_CALLABLE_COUNT; i++) {
        calynda_rt_register_static_object(&ALL_GFX_CALLABLES[i]->header);
    }
}

/* ------------------------------------------------------------------ */
/* Dispatch                                                             */
/* ------------------------------------------------------------------ */

#ifdef CALYNDA_WII_BUILD
/* Persistent bridge state for Wii */
static Gfx3dCamera       g_camera;
static Gfx3dLight        g_lights[8];
static int               g_light_count;
static Gfx3dMaterial     g_material;
static Gfx3dSceneNode   *g_nodes[64];
static int               g_node_count;
static Gfx3dParticleSystem g_particles;
static float             g_time;
#endif

CalyndaRtWord calynda_gfx_dispatch(const CalyndaRtExternCallable *callable,
                                    size_t argument_count,
                                    const CalyndaRtWord *arguments) {
    (void)argument_count;
    (void)arguments;

    switch ((int)callable->kind) {

    /* ---- Init / frame ---- */
    case GFX_CALL_INIT:
#ifdef CALYNDA_WII_BUILD
        gfx3d_init();
        g_camera = gfx3d_camera_default(
            gfx3d_get_display_info().aspect);
        g_light_count = 0;
        g_node_count  = 0;
        g_material    = gfx3d_material_default();
        gfx3d_particles_init(&g_particles);
        g_time = 0;
#else
        fprintf(stderr, "[gfx.init] stub on host\n");
#endif
        return 0;

    case GFX_CALL_FRAME_BEGIN:
#ifdef CALYNDA_WII_BUILD
        gfx3d_frame_begin();
#else
        fprintf(stderr, "[gfx.frame_begin] stub\n");
#endif
        return 0;

    case GFX_CALL_FRAME_END:
#ifdef CALYNDA_WII_BUILD
        gfx3d_frame_end();
#else
        fprintf(stderr, "[gfx.frame_end] stub\n");
#endif
        return 0;

    case GFX_CALL_SETUP_3D:
#ifdef CALYNDA_WII_BUILD
        gfx3d_setup_3d_mode();
#else
        fprintf(stderr, "[gfx.setup_3d] stub\n");
#endif
        return 0;

    case GFX_CALL_SETUP_2D:
#ifdef CALYNDA_WII_BUILD
        gfx3d_setup_2d_mode();
#else
        fprintf(stderr, "[gfx.setup_2d] stub\n");
#endif
        return 0;

    /* ---- Camera ---- */
    case GFX_CALL_CAMERA_SET_POS:
#ifdef CALYNDA_WII_BUILD
        g_camera.position = (CmVec3){
            argument_count > 0 ? word_to_float(arguments[0]) : 0,
            argument_count > 1 ? word_to_float(arguments[1]) : 0,
            argument_count > 2 ? word_to_float(arguments[2]) : 0 };
#else
        fprintf(stderr, "[gfx.camera_set_pos] stub\n");
#endif
        return 0;

    case GFX_CALL_CAMERA_SET_TARGET:
#ifdef CALYNDA_WII_BUILD
        g_camera.target = (CmVec3){
            argument_count > 0 ? word_to_float(arguments[0]) : 0,
            argument_count > 1 ? word_to_float(arguments[1]) : 0,
            argument_count > 2 ? word_to_float(arguments[2]) : 0 };
#else
        fprintf(stderr, "[gfx.camera_set_target] stub\n");
#endif
        return 0;

    case GFX_CALL_CAMERA_SET_FOV:
#ifdef CALYNDA_WII_BUILD
        g_camera.fov_y = argument_count > 0 ? word_to_float(arguments[0]) : 60;
#else
        fprintf(stderr, "[gfx.camera_set_fov] stub\n");
#endif
        return 0;

    case GFX_CALL_CAMERA_APPLY:
#ifdef CALYNDA_WII_BUILD
        gfx3d_camera_apply(&g_camera);
#else
        fprintf(stderr, "[gfx.camera_apply] stub\n");
#endif
        return 0;

    case GFX_CALL_CAMERA_ORBIT_UPDATE:
#ifdef CALYNDA_WII_BUILD
        gfx3d_camera_orbit_rotate(&g_camera,
            argument_count > 0 ? word_to_float(arguments[0]) : 0,
            argument_count > 1 ? word_to_float(arguments[1]) : 0);
#else
        fprintf(stderr, "[gfx.camera_orbit_update] stub\n");
#endif
        return 0;

    case GFX_CALL_CAMERA_FOLLOW_UPDATE:
#ifdef CALYNDA_WII_BUILD
    {
        CmVec3 tgt = {
            argument_count > 0 ? word_to_float(arguments[0]) : 0,
            argument_count > 1 ? word_to_float(arguments[1]) : 0,
            argument_count > 2 ? word_to_float(arguments[2]) : 0 };
        gfx3d_camera_follow_update(&g_camera, tgt);
    }
#else
        fprintf(stderr, "[gfx.camera_follow_update] stub\n");
#endif
        return 0;

    /* ---- Mesh ---- */
    case GFX_CALL_MESH_CREATE_CUBE:
#ifdef CALYNDA_WII_BUILD
        return (CalyndaRtWord)(uintptr_t)gfx3d_mesh_create_cube();
#else
        fprintf(stderr, "[gfx.mesh_create_cube] stub → handle 1\n");
        return 1;
#endif

    case GFX_CALL_MESH_CREATE_SPHERE:
#ifdef CALYNDA_WII_BUILD
    {
        float r = argument_count > 0 ? word_to_float(arguments[0]) : 1;
        int slices = argument_count > 1 ? (int)arguments[1] : 12;
        int stacks = argument_count > 2 ? (int)arguments[2] : 8;
        return (CalyndaRtWord)(uintptr_t)gfx3d_mesh_create_sphere(r, slices, stacks);
    }
#else
        fprintf(stderr, "[gfx.mesh_create_sphere] stub → handle 2\n");
        return 2;
#endif

    case GFX_CALL_MESH_CREATE_PLANE:
#ifdef CALYNDA_WII_BUILD
    {
        float w = argument_count > 0 ? word_to_float(arguments[0]) : 10;
        float d = argument_count > 1 ? word_to_float(arguments[1]) : 10;
        int sx  = argument_count > 2 ? (int)arguments[2] : 1;
        int sz  = argument_count > 3 ? (int)arguments[3] : 1;
        return (CalyndaRtWord)(uintptr_t)gfx3d_mesh_create_plane(w, d, sx, sz);
    }
#else
        fprintf(stderr, "[gfx.mesh_create_plane] stub → handle 3\n");
        return 3;
#endif

    case GFX_CALL_MESH_DRAW:
#ifdef CALYNDA_WII_BUILD
    {
        Gfx3dMesh *m = (Gfx3dMesh *)(uintptr_t)(
            argument_count > 0 ? arguments[0] : 0);
        if (m) gfx3d_mesh_draw(m);
    }
#else
        fprintf(stderr, "[gfx.mesh_draw] stub\n");
#endif
        return 0;

    /* ---- Material ---- */
    case GFX_CALL_MATERIAL_SET_MODE:
#ifdef CALYNDA_WII_BUILD
        g_material.mode = (Gfx3dMaterialMode)(
            argument_count > 0 ? (int)arguments[0] : 0);
#else
        fprintf(stderr, "[gfx.material_set_mode] stub\n");
#endif
        return 0;

    case GFX_CALL_MATERIAL_APPLY:
#ifdef CALYNDA_WII_BUILD
    {
        if (argument_count >= 3) {
            uint8_t r = (uint8_t)(word_to_float(arguments[0]) * 255);
            uint8_t g = (uint8_t)(word_to_float(arguments[1]) * 255);
            uint8_t b = (uint8_t)(word_to_float(arguments[2]) * 255);
            g_material.color[0] = r;
            g_material.color[1] = g;
            g_material.color[2] = b;
            g_material.color[3] = 255;
        }
        gfx3d_material_apply(&g_material);
    }
#else
        fprintf(stderr, "[gfx.material_apply] stub\n");
#endif
        return 0;

    /* ---- Lights ---- */
    case GFX_CALL_LIGHT_SET_AMBIENT:
#ifdef CALYNDA_WII_BUILD
    {
        Gfx3dAmbient amb;
        amb.r = argument_count > 0 ? (uint8_t)(word_to_float(arguments[0]) * 255) : 0;
        amb.g = argument_count > 1 ? (uint8_t)(word_to_float(arguments[1]) * 255) : 0;
        amb.b = argument_count > 2 ? (uint8_t)(word_to_float(arguments[2]) * 255) : 0;
        gfx3d_set_ambient(amb);
    }
#else
        fprintf(stderr, "[gfx.light_set_ambient] stub\n");
#endif
        return 0;

    case GFX_CALL_LIGHT_ADD_DIRECTIONAL:
#ifdef CALYNDA_WII_BUILD
        if (g_light_count < 8) {
            CmVec3 dir = {
                argument_count > 0 ? word_to_float(arguments[0]) : 0,
                argument_count > 1 ? word_to_float(arguments[1]) : -1,
                argument_count > 2 ? word_to_float(arguments[2]) : 0 };
            Gfx3dColor col;
            col.r = argument_count > 3 ? (uint8_t)(word_to_float(arguments[3]) * 255) : 255;
            col.g = argument_count > 4 ? (uint8_t)(word_to_float(arguments[4]) * 255) : 255;
            col.b = argument_count > 5 ? (uint8_t)(word_to_float(arguments[5]) * 255) : 255;
            col.a = 255;
            g_lights[g_light_count++] = gfx3d_light_directional(dir, col, 1.0f);
        }
#else
        fprintf(stderr, "[gfx.light_add_directional] stub\n");
#endif
        return 0;

    case GFX_CALL_LIGHT_ADD_POINT:
#ifdef CALYNDA_WII_BUILD
        if (g_light_count < 8) {
            CmVec3 pos = {
                argument_count > 0 ? word_to_float(arguments[0]) : 0,
                argument_count > 1 ? word_to_float(arguments[1]) : 0,
                argument_count > 2 ? word_to_float(arguments[2]) : 0 };
            Gfx3dColor col;
            col.r = argument_count > 3 ? (uint8_t)(word_to_float(arguments[3]) * 255) : 255;
            col.g = argument_count > 4 ? (uint8_t)(word_to_float(arguments[4]) * 255) : 255;
            col.b = argument_count > 5 ? (uint8_t)(word_to_float(arguments[5]) * 255) : 255;
            col.a = 255;
            float range = argument_count > 6 ? word_to_float(arguments[6]) : 10;
            g_lights[g_light_count++] = gfx3d_light_point(pos, col, 1.0f, range);
        }
#else
        fprintf(stderr, "[gfx.light_add_point] stub\n");
#endif
        return 0;

    case GFX_CALL_LIGHTS_APPLY:
#ifdef CALYNDA_WII_BUILD
        gfx3d_lights_apply(g_lights, g_light_count,
                           gfx3d_camera_view(&g_camera));
#else
        fprintf(stderr, "[gfx.lights_apply] stub\n");
#endif
        return 0;

    /* ---- Scene ---- */
    case GFX_CALL_SCENE_CREATE_NODE:
#ifdef CALYNDA_WII_BUILD
    {
        Gfx3dSceneNode *node = gfx3d_node_create("node");
        if (g_node_count < 64) g_nodes[g_node_count++] = node;
        return (CalyndaRtWord)(uintptr_t)node;
    }
#else
        fprintf(stderr, "[gfx.scene_create_node] stub → handle 100\n");
        return 100;
#endif

    case GFX_CALL_NODE_SET_POS:
#ifdef CALYNDA_WII_BUILD
    {
        int idx = argument_count > 0 ? (int)arguments[0] : 0;
        if (idx >= 0 && idx < g_node_count && g_nodes[idx]) {
            g_nodes[idx]->position = (CmVec3){
                argument_count > 1 ? word_to_float(arguments[1]) : 0,
                argument_count > 2 ? word_to_float(arguments[2]) : 0,
                argument_count > 3 ? word_to_float(arguments[3]) : 0 };
        }
    }
#else
        fprintf(stderr, "[gfx.node_set_pos] stub\n");
#endif
        return 0;

    case GFX_CALL_NODE_SET_ROTATION:
#ifdef CALYNDA_WII_BUILD
    {
        int idx = argument_count > 0 ? (int)arguments[0] : 0;
        if (idx >= 0 && idx < g_node_count && g_nodes[idx]) {
            g_nodes[idx]->rotation = (CmQuat){
                argument_count > 1 ? word_to_float(arguments[1]) : 0,
                argument_count > 2 ? word_to_float(arguments[2]) : 0,
                argument_count > 3 ? word_to_float(arguments[3]) : 0,
                argument_count > 4 ? word_to_float(arguments[4]) : 1 };
        }
    }
#else
        fprintf(stderr, "[gfx.node_set_rotation] stub\n");
#endif
        return 0;

    case GFX_CALL_NODE_SET_SCALE:
#ifdef CALYNDA_WII_BUILD
    {
        int idx = argument_count > 0 ? (int)arguments[0] : 0;
        if (idx >= 0 && idx < g_node_count && g_nodes[idx]) {
            g_nodes[idx]->scale = (CmVec3){
                argument_count > 1 ? word_to_float(arguments[1]) : 1,
                argument_count > 2 ? word_to_float(arguments[2]) : 1,
                argument_count > 3 ? word_to_float(arguments[3]) : 1 };
        }
    }
#else
        fprintf(stderr, "[gfx.node_set_scale] stub\n");
#endif
        return 0;

    case GFX_CALL_NODE_SET_MESH:
#ifdef CALYNDA_WII_BUILD
    {
        int idx = argument_count > 0 ? (int)arguments[0] : 0;
        Gfx3dMesh *m = argument_count > 1
            ? (Gfx3dMesh *)(uintptr_t)arguments[1] : NULL;
        if (idx >= 0 && idx < g_node_count && g_nodes[idx] && m) {
            gfx3d_node_set_mesh(g_nodes[idx], m, g_material);
        }
    }
#else
        fprintf(stderr, "[gfx.node_set_mesh] stub\n");
#endif
        return 0;

    case GFX_CALL_SCENE_RENDER:
#ifdef CALYNDA_WII_BUILD
    {
        CmMat4 ident = cm_mat4_identity();
        CmMat4 view = gfx3d_camera_view(&g_camera);
        for (int i = 0; i < g_node_count; i++) {
            if (!g_nodes[i]) continue;
            gfx3d_node_update_transforms(g_nodes[i], ident);
            if (g_nodes[i]->mesh && g_nodes[i]->has_material) {
                gfx3d_material_apply(&g_nodes[i]->material);
                CmMat4 modelview = cm_mat4_mul(view, g_nodes[i]->world_matrix);
                gfx3d_load_model_matrix(modelview);
                gfx3d_mesh_draw(g_nodes[i]->mesh);
            }
        }
    }
#else
        fprintf(stderr, "[gfx.scene_render] stub\n");
#endif
        return 0;

    /* ---- Skybox ---- */
    case GFX_CALL_SKYBOX_DRAW:
#ifdef CALYNDA_WII_BUILD
        /* Skybox requires 6 textures — no-op if not loaded */
#else
        fprintf(stderr, "[gfx.skybox_draw] stub\n");
#endif
        return 0;

    /* ---- Particles ---- */
    case GFX_CALL_PARTICLES_EMIT:
#ifdef CALYNDA_WII_BUILD
    {
        CmVec3 pos = {
            argument_count > 0 ? word_to_float(arguments[0]) : 0,
            argument_count > 1 ? word_to_float(arguments[1]) : 0,
            argument_count > 2 ? word_to_float(arguments[2]) : 0 };
        int count = argument_count > 3 ? (int)arguments[3] : 10;
        Gfx3dParticleEmitter em = gfx3d_emitter_sparks(pos);
        gfx3d_particles_emit(&g_particles, &em, count);
    }
#else
        fprintf(stderr, "[gfx.particles_emit] stub\n");
#endif
        return 0;

    case GFX_CALL_PARTICLES_UPDATE:
#ifdef CALYNDA_WII_BUILD
    {
        float dt = argument_count > 0 ? word_to_float(arguments[0]) : (1.0f / 60);
        Gfx3dParticleEmitter em = gfx3d_emitter_sparks((CmVec3){0, 0, 0});
        gfx3d_particles_update(&g_particles, dt, &em);
        g_time += dt;
    }
#else
        fprintf(stderr, "[gfx.particles_update] stub\n");
#endif
        return 0;

    case GFX_CALL_PARTICLES_DRAW:
#ifdef CALYNDA_WII_BUILD
    {
        CmMat4 view = gfx3d_camera_view(&g_camera);
        CmVec3 right = { view.m[0], view.m[4], view.m[8] };
        CmVec3 up    = { view.m[1], view.m[5], view.m[9] };
        gfx3d_particles_draw(&g_particles, right, up, NULL);
    }
#else
        fprintf(stderr, "[gfx.particles_draw] stub\n");
#endif
        return 0;

    /* ---- Water ---- */
    case GFX_CALL_WATER_UPDATE:
#ifdef CALYNDA_WII_BUILD
        /* Water requires a Gfx3dWater object — no-op for bridge demo */
#else
        fprintf(stderr, "[gfx.water_update] stub\n");
#endif
        return 0;

    case GFX_CALL_WATER_DRAW:
#ifdef CALYNDA_WII_BUILD
        /* no-op — needs Gfx3dWater object */
#else
        fprintf(stderr, "[gfx.water_draw] stub\n");
#endif
        return 0;
    }

    fprintf(stderr, "runtime: unsupported gfx callable kind %d\n",
            (int)callable->kind);
    return 0;
}

/* ------------------------------------------------------------------ */
/* Member Load                                                          */
/* ------------------------------------------------------------------ */

static CalyndaRtWord rt_make_obj(void *p) {
    return (CalyndaRtWord)(uintptr_t)p;
}

CalyndaRtWord calynda_gfx_member_load(const char *member) {
    if (!member) return 0;

    if (strcmp(member, "init")            == 0) return rt_make_obj(&GFX_INIT_CALLABLE);
    if (strcmp(member, "frame_begin")     == 0) return rt_make_obj(&GFX_FRAME_BEGIN_CALLABLE);
    if (strcmp(member, "frame_end")       == 0) return rt_make_obj(&GFX_FRAME_END_CALLABLE);
    if (strcmp(member, "setup_3d")        == 0) return rt_make_obj(&GFX_SETUP_3D_CALLABLE);
    if (strcmp(member, "setup_2d")        == 0) return rt_make_obj(&GFX_SETUP_2D_CALLABLE);
    if (strcmp(member, "camera_set_pos")  == 0) return rt_make_obj(&GFX_CAMERA_SET_POS_CALLABLE);
    if (strcmp(member, "camera_set_target") == 0) return rt_make_obj(&GFX_CAMERA_SET_TARGET_CALLABLE);
    if (strcmp(member, "camera_set_fov")  == 0) return rt_make_obj(&GFX_CAMERA_SET_FOV_CALLABLE);
    if (strcmp(member, "camera_apply")    == 0) return rt_make_obj(&GFX_CAMERA_APPLY_CALLABLE);
    if (strcmp(member, "camera_orbit_update") == 0) return rt_make_obj(&GFX_CAMERA_ORBIT_CALLABLE);
    if (strcmp(member, "camera_follow_update") == 0) return rt_make_obj(&GFX_CAMERA_FOLLOW_CALLABLE);
    if (strcmp(member, "mesh_create_cube") == 0) return rt_make_obj(&GFX_MESH_CUBE_CALLABLE);
    if (strcmp(member, "mesh_create_sphere") == 0) return rt_make_obj(&GFX_MESH_SPHERE_CALLABLE);
    if (strcmp(member, "mesh_create_plane") == 0) return rt_make_obj(&GFX_MESH_PLANE_CALLABLE);
    if (strcmp(member, "mesh_draw")       == 0) return rt_make_obj(&GFX_MESH_DRAW_CALLABLE);
    if (strcmp(member, "material_set_mode") == 0) return rt_make_obj(&GFX_MAT_MODE_CALLABLE);
    if (strcmp(member, "material_apply")  == 0) return rt_make_obj(&GFX_MAT_APPLY_CALLABLE);
    if (strcmp(member, "light_set_ambient") == 0) return rt_make_obj(&GFX_LIGHT_AMBIENT_CALLABLE);
    if (strcmp(member, "light_add_directional") == 0) return rt_make_obj(&GFX_LIGHT_DIR_CALLABLE);
    if (strcmp(member, "light_add_point") == 0) return rt_make_obj(&GFX_LIGHT_POINT_CALLABLE);
    if (strcmp(member, "lights_apply")    == 0) return rt_make_obj(&GFX_LIGHTS_APPLY_CALLABLE);
    if (strcmp(member, "scene_create_node") == 0) return rt_make_obj(&GFX_SCENE_NODE_CALLABLE);
    if (strcmp(member, "node_set_pos")    == 0) return rt_make_obj(&GFX_NODE_SET_POS_CALLABLE);
    if (strcmp(member, "node_set_rotation") == 0) return rt_make_obj(&GFX_NODE_SET_ROT_CALLABLE);
    if (strcmp(member, "node_set_scale")  == 0) return rt_make_obj(&GFX_NODE_SET_SCALE_CALLABLE);
    if (strcmp(member, "node_set_mesh")   == 0) return rt_make_obj(&GFX_NODE_SET_MESH_CALLABLE);
    if (strcmp(member, "scene_render")    == 0) return rt_make_obj(&GFX_SCENE_RENDER_CALLABLE);
    if (strcmp(member, "skybox_draw")     == 0) return rt_make_obj(&GFX_SKYBOX_DRAW_CALLABLE);
    if (strcmp(member, "particles_emit")  == 0) return rt_make_obj(&GFX_PARTICLES_EMIT_CALLABLE);
    if (strcmp(member, "particles_update") == 0) return rt_make_obj(&GFX_PARTICLES_UPDATE_CALLABLE);
    if (strcmp(member, "particles_draw")  == 0) return rt_make_obj(&GFX_PARTICLES_DRAW_CALLABLE);
    if (strcmp(member, "water_update")    == 0) return rt_make_obj(&GFX_WATER_UPDATE_CALLABLE);
    if (strcmp(member, "water_draw")      == 0) return rt_make_obj(&GFX_WATER_DRAW_CALLABLE);

    fprintf(stderr, "runtime: gfx has no member '%s'\n", member);
    return 0;
}
