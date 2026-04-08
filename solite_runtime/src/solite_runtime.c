/*
 * solid_runtime.c — Top-level Solite/Wii runtime main loop.
 *
 * Integrates the reactive system with libwiigui's Update()/Draw() cycle.
 * Each frame:
 *   1. Scan input (WPAD/PAD)
 *   2. Flush pending reactive effects
 *   3. Update GUI tree (input handling)
 *   4. Draw GUI tree
 *   5. Present frame (VIDEO_WaitVSync)
 */

#include "solite_runtime.h"
#include "solite_gui_bridge.h"
#include "solite_effect.h"
#include "calynda_runtime.h"

static bool    runtime_initialized = false;
static bool    runtime_mounted     = false;
static bool    unmount_requested   = false;

/* Pointer cursor tracking — up to 4 Wii remotes */
#define SOLITE_MAX_POINTERS 4
static SoliteGuiElement *pointer_cursors[SOLITE_MAX_POINTERS] = {NULL};
static bool             pointer_registered[SOLITE_MAX_POINTERS] = {false};

void solite_runtime_init(void)
{
    if (runtime_initialized) return;
    calynda_rt_init();
    runtime_initialized = true;
}

void solite_runtime_shutdown(void)
{
    runtime_initialized = false;
    runtime_mounted = false;
    unmount_requested = false;
}

bool solite_is_mounted(void)
{
    return runtime_mounted;
}

void solite_request_unmount(void)
{
    unmount_requested = true;
}

void solite_register_pointer(int channel, SoliteGuiElement *cursor)
{
    if (channel < 0 || channel >= SOLITE_MAX_POINTERS) return;
    pointer_cursors[channel] = cursor;
    pointer_registered[channel] = (cursor != NULL);
}

static void solite_update_pointers(void)
{
    for (int ch = 0; ch < SOLITE_MAX_POINTERS; ch++) {
        if (!pointer_registered[ch]) continue;
        int px, py;
        float angle;
        bool valid;
        solite_bridge_pointer_get_position(ch, &px, &py, &angle, &valid);
        if (valid) {
            solite_bridge_element_set_visible(pointer_cursors[ch], true);
            solite_bridge_element_set_position(pointer_cursors[ch], px, py);
            solite_bridge_element_set_angle(pointer_cursors[ch], angle);
        } else {
            solite_bridge_element_set_visible(pointer_cursors[ch], false);
        }
    }
}

void solite_mount(SoliteGuiWindow *root)
{
    if (!root) return;

    solite_runtime_init();
    runtime_mounted = true;
    unmount_requested = false;

    /* Initialize video and input via bridge */
    solite_bridge_video_init();
    solite_bridge_input_init();

    while (!unmount_requested) {
        /* 1. Scan input */
        solite_bridge_input_scan();

        /* 2. Update pointer cursors from IR data */
        solite_update_pointers();

        /* 3. Flush any pending reactive updates */
        solite_effect_flush_pending();

        /* 4. Update GUI tree with current input */
        solite_bridge_window_update(root);

        /* 5. Poll registered button click handlers */
        solite_bridge_poll_button_clicks();

        /* 6. Draw */
        solite_bridge_draw_start();
        solite_bridge_window_draw(root);
        solite_bridge_draw_end();
    }

    runtime_mounted = false;
    solite_runtime_shutdown();
}
