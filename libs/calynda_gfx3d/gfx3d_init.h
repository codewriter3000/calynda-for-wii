/*
 * gfx3d_init.h — Video/GX initialization and frame management for 3D.
 *
 * Extends the base video init with perspective projection mode,
 * frame begin/end with clear+swap+vsync, and viewport control.
 */

#ifndef GFX3D_INIT_H
#define GFX3D_INIT_H

#include <stdbool.h>
#include <stdint.h>

#ifdef CALYNDA_WII_BUILD
#include <gccore.h>
#endif

/* Clear color for the background */
typedef struct {
    uint8_t r, g, b, a;
} Gfx3dColor;

#define GFX3D_COLOR_BLACK   ((Gfx3dColor){ 0, 0, 0, 255 })
#define GFX3D_COLOR_WHITE   ((Gfx3dColor){ 255, 255, 255, 255 })
#define GFX3D_COLOR_SKY     ((Gfx3dColor){ 100, 149, 237, 255 })

/* Screen mode info */
typedef struct {
    int      width;
    int      height;
    bool     widescreen;
    float    aspect;
} Gfx3dDisplayInfo;

/*
 * Initialize the 3D rendering subsystem.
 * Sets up VIDEO, GX, double-buffered framebuffers, and FIFO.
 * Must be called once at startup before any other gfx3d calls.
 */
void gfx3d_init(void);

/* Get display information (populated after gfx3d_init) */
Gfx3dDisplayInfo gfx3d_get_display_info(void);

/* Set the background clear color */
void gfx3d_set_clear_color(Gfx3dColor color);

/*
 * Begin a new frame. Clears color+depth buffers.
 * All 3D draw calls go between begin and end.
 */
void gfx3d_frame_begin(void);

/*
 * End the frame. Copies EFB → XFB, flips framebuffer, waits for vsync.
 */
void gfx3d_frame_end(void);

/*
 * Configure the GX pipeline for 3D rendering:
 * vertex format with pos/normal/uv/color, depth testing, etc.
 */
void gfx3d_setup_3d_mode(void);

/*
 * Reset to 2D ortho mode (for HUD/UI overlay via Solite).
 * Called after 3D scene is drawn, before Solite draws its UI.
 */
void gfx3d_setup_2d_mode(void);

/* Shutdown the graphics subsystem */
void gfx3d_shutdown(void);

#endif /* GFX3D_INIT_H */
