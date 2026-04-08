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

/* Pointer cursor tracking — up to 4 Wii remotes */
#define SOLID_MAX_POINTERS 4
static SolidGuiElement *pointer_cursors[SOLID_MAX_POINTERS] = {NULL};
static bool             pointer_registered[SOLID_MAX_POINTERS] = {false};

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

void solid_register_pointer(int channel, SolidGuiElement *cursor)
{
    if (channel < 0 || channel >= SOLID_MAX_POINTERS) return;
    pointer_cursors[channel] = cursor;
    pointer_registered[channel] = (cursor != NULL);
}

static void solid_update_pointers(void)
{
    for (int ch = 0; ch < SOLID_MAX_POINTERS; ch++) {
        if (!pointer_registered[ch]) continue;
        int px, py;
        float angle;
        bool valid;
        solid_bridge_pointer_get_position(ch, &px, &py, &angle, &valid);
        if (valid) {
            solid_bridge_element_set_visible(pointer_cursors[ch], true);
            solid_bridge_element_set_position(pointer_cursors[ch], px, py);
            solid_bridge_element_set_angle(pointer_cursors[ch], angle);
        } else {
            solid_bridge_element_set_visible(pointer_cursors[ch], false);
        }
    }
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

        /* 2. Update pointer cursors from IR data */
        solid_update_pointers();

        /* 3. Flush any pending reactive updates */
        solid_effect_flush_pending();

        /* 4. Update GUI tree with current input */
        solid_bridge_window_update(root);

        /* 5. Poll registered button click handlers */
        solid_bridge_poll_button_clicks();

        /* 6. Draw */
        solid_bridge_draw_start();
        solid_bridge_window_draw(root);
        solid_bridge_draw_end();
    }

    runtime_mounted = false;
    solid_runtime_shutdown();
}
