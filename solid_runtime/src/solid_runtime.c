/*
 * solid_runtime.c — Top-level Solid/Wii runtime main loop.
 *
 * Integrates the reactive system with libwiigui's Update()/Draw() cycle.
 * Each frame:
 *   1. Scan input (WPAD/PAD)
 *   2. Flush pending reactive effects
 *   3. Update GUI tree (input handling)
 *   4. Draw GUI tree
 *   5. Present frame (VIDEO_WaitVSync)
 */

#include "solid_runtime.h"
#include "solid_gui_bridge.h"
#include "solid_effect.h"
#include "calynda_runtime.h"

static bool    runtime_initialized = false;
static bool    runtime_mounted     = false;
static bool    unmount_requested   = false;

void solid_runtime_init(void)
{
    if (runtime_initialized) return;
    calynda_rt_init();
    runtime_initialized = true;
}

void solid_runtime_shutdown(void)
{
    runtime_initialized = false;
    runtime_mounted = false;
    unmount_requested = false;
}

bool solid_is_mounted(void)
{
    return runtime_mounted;
}

void solid_request_unmount(void)
{
    unmount_requested = true;
}

void solid_mount(SolidGuiWindow *root)
{
    if (!root) return;

    solid_runtime_init();
    runtime_mounted = true;
    unmount_requested = false;

    /* Initialize video and input via bridge */
    solid_bridge_video_init();
    solid_bridge_input_init();

    while (!unmount_requested) {
        /* 1. Scan input */
        solid_bridge_input_scan();

        /* 2. Flush any pending reactive updates */
        solid_effect_flush_pending();

        /* 3. Update GUI tree with current input */
        solid_bridge_window_update(root);

        /* 4. Draw */
        solid_bridge_draw_start();
        solid_bridge_window_draw(root);
        solid_bridge_draw_end();
    }

    runtime_mounted = false;
    solid_runtime_shutdown();
}
