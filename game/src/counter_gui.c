/*
 * counter_gui.c — Minimal reactive counter with Wii remote pointer.
 *
 * This is the C output that the Calynda JSX compiler would generate
 * from counter.cal. It uses the Solid/Wii reactive runtime + bridge.
 *
 * Layout (640×480):
 *   - Dark background
 *   - "Count: N" text (reactive, updates on signal change)
 *   - [+] button  — increments counter
 *   - [-] button  — decrements counter
 *   - [Reset] button — resets to 0
 *   - Wii remote pointer cursor (small crosshair image, tracks IR)
 */

#include "solid_signal.h"
#include "solid_effect.h"
#include "solid_gui_bridge.h"
#include "calynda_runtime.h"

#include <stdio.h>
#include <string.h>

/* ════════════════════════════════════════════════════════════════════
 *  Signals
 * ════════════════════════════════════════════════════════════════════ */

static SolidSignal *sig_count = NULL;   /* int counter value     */

/* ════════════════════════════════════════════════════════════════════
 *  GUI handles
 * ════════════════════════════════════════════════════════════════════ */

static SolidGuiWindow *root_window   = NULL;

/* Background */
static SolidGuiImage  *bg_image      = NULL;

/* Counter display */
static SolidGuiText   *count_label   = NULL;

/* Buttons */
static SolidGuiButton *btn_inc       = NULL;
static SolidGuiText   *btn_inc_lbl   = NULL;
static SolidGuiButton *btn_dec       = NULL;
static SolidGuiText   *btn_dec_lbl   = NULL;
static SolidGuiButton *btn_reset     = NULL;
static SolidGuiText   *btn_reset_lbl = NULL;

/* Pointer cursor */
static SolidGuiImage  *cursor_img    = NULL;

/* ════════════════════════════════════════════════════════════════════
 *  Reactive effect: update counter text when sig_count changes
 * ════════════════════════════════════════════════════════════════════ */

static void counter_display_effect(void *ctx)
{
    (void)ctx;
    CalyndaRtWord val = solid_signal_get(sig_count);
    int n = (int)val;

    /* Format "Count: N" into a stack buffer */
    char buf[64];
    snprintf(buf, sizeof(buf), "Count: %d", n);

    solid_bridge_text_set_text(count_label, buf);
}

/* ════════════════════════════════════════════════════════════════════
 *  Build the GUI tree (what JSX emit would generate)
 * ════════════════════════════════════════════════════════════════════ */

static SolidGuiWindow *build_counter_component(void)
{
    /* ── Root window (640×480, full screen) ── */
    root_window = solid_bridge_window_new(640, 480);

    /* ── Background: dark grey rectangle ── */
    bg_image = solid_bridge_image_new_color(640, 480, 0x2D, 0x2D, 0x2D, 0xFF);
    solid_bridge_window_append(root_window,
        solid_bridge_image_as_element(bg_image));

    /* ── Title text ── */
    SolidGuiText *title = solid_bridge_text_new("Calynda Counter", 30,
                                                 0x00, 0xCC, 0xFF, 0xFF);
    solid_bridge_element_set_position(
        solid_bridge_text_as_element(title), 200, 40);
    solid_bridge_window_append(root_window,
        solid_bridge_text_as_element(title));

    /* ── Counter value label ── */
    count_label = solid_bridge_text_new("Count: 0", 36,
                                         0xFF, 0xFF, 0xFF, 0xFF);
    solid_bridge_element_set_position(
        solid_bridge_text_as_element(count_label), 230, 140);
    solid_bridge_window_append(root_window,
        solid_bridge_text_as_element(count_label));

    /* ── [ + ] button ── */
    btn_inc = solid_bridge_button_new(120, 60);
    solid_bridge_element_set_position(
        solid_bridge_button_as_element(btn_inc), 140, 250);
    btn_inc_lbl = solid_bridge_text_new("+", 28, 0xFF, 0xFF, 0xFF, 0xFF);
    solid_bridge_button_set_label(btn_inc, btn_inc_lbl);
    solid_bridge_button_set_trigger(btn_inc);
    solid_bridge_window_append(root_window,
        solid_bridge_button_as_element(btn_inc));

    /* ── [ - ] button ── */
    btn_dec = solid_bridge_button_new(120, 60);
    solid_bridge_element_set_position(
        solid_bridge_button_as_element(btn_dec), 260, 250);
    btn_dec_lbl = solid_bridge_text_new("-", 28, 0xFF, 0xFF, 0xFF, 0xFF);
    solid_bridge_button_set_label(btn_dec, btn_dec_lbl);
    solid_bridge_button_set_trigger(btn_dec);
    solid_bridge_window_append(root_window,
        solid_bridge_button_as_element(btn_dec));

    /* ── [ Reset ] button ── */
    btn_reset = solid_bridge_button_new(120, 60);
    solid_bridge_element_set_position(
        solid_bridge_button_as_element(btn_reset), 380, 250);
    btn_reset_lbl = solid_bridge_text_new("Reset", 22, 0xFF, 0xFF, 0xFF, 0xFF);
    solid_bridge_button_set_label(btn_reset, btn_reset_lbl);
    solid_bridge_button_set_trigger(btn_reset);
    solid_bridge_window_append(root_window,
        solid_bridge_button_as_element(btn_reset));

    /* ── Pointer cursor (small white crosshair square) ── */
    cursor_img = solid_bridge_image_new_color(48, 48, 0xFF, 0xFF, 0xFF, 0xCC);
    solid_bridge_element_set_position(
        solid_bridge_image_as_element(cursor_img), 320, 240);
    /* Cursor is appended LAST so it draws on top of everything */
    solid_bridge_window_append(root_window,
        solid_bridge_image_as_element(cursor_img));

    return root_window;
}

/* ════════════════════════════════════════════════════════════════════
 *  Per-frame: poll buttons & update pointer position
 * ════════════════════════════════════════════════════════════════════ */

static void frame_poll(void)
{
    /* ── Button click checks (poll-based) ── */
    if (solid_bridge_button_was_clicked(btn_inc)) {
        CalyndaRtWord cur = solid_signal_get(sig_count);
        solid_signal_set(sig_count, cur + 1);
    }
    if (solid_bridge_button_was_clicked(btn_dec)) {
        CalyndaRtWord cur = solid_signal_get(sig_count);
        solid_signal_set(sig_count, cur > 0 ? cur - 1 : 0);
    }
    if (solid_bridge_button_was_clicked(btn_reset)) {
        solid_signal_set(sig_count, (CalyndaRtWord)0);
    }

    /* ── Update pointer cursor position from Wii remote IR ── */
    int px = 0, py = 0;
    float angle = 0.0f;
    bool valid = false;

    solid_bridge_pointer_get_position(0, &px, &py, &angle, &valid);

    SolidGuiElement *cursor_el = solid_bridge_image_as_element(cursor_img);

    if (valid) {
        /* Center the 48×48 cursor image on the pointer position */
        solid_bridge_element_set_position(cursor_el, px - 24, py - 24);
        solid_bridge_element_set_angle(cursor_el, angle);
        solid_bridge_element_set_visible(cursor_el, true);
    } else {
        solid_bridge_element_set_visible(cursor_el, false);
    }
}

/* ════════════════════════════════════════════════════════════════════
 *  Main loop (replaces solid_mount for this standalone app)
 * ════════════════════════════════════════════════════════════════════ */

static bool app_running = true;

static void counter_main_loop(SolidGuiWindow *root)
{
    solid_bridge_video_init();
    solid_bridge_input_init();

    while (app_running) {
        /* 1. Scan input */
        solid_bridge_input_scan();

        /* 2. Poll GUI buttons & update pointer */
        frame_poll();

        /* 3. Flush reactive effects (counter label update, etc.) */
        solid_effect_flush_pending();

        /* 4. Update GUI tree with input (hit testing) */
        solid_bridge_window_update(root);

        /* 5. Draw */
        solid_bridge_draw_start();
        solid_bridge_window_draw(root);
        solid_bridge_draw_end();
    }
}

/* ════════════════════════════════════════════════════════════════════
 *  Entry point (called from calynda boot or standalone main)
 * ════════════════════════════════════════════════════════════════════ */

void counter_app_start(void)
{
    /* Create signals */
    sig_count = solid_create_signal((CalyndaRtWord)0);

    /* Build GUI tree */
    SolidGuiWindow *root = build_counter_component();

    /* Create reactive effect: update label whenever sig_count changes */
    solid_create_effect(counter_display_effect, NULL);

    /* Enter main loop */
    counter_main_loop(root);

    /* Cleanup */
    solid_signal_dispose(sig_count);
}

/*
 * Standalone main() for host testing.
 * On Wii, calynda_wii_main.c provides main() and calls boot().
 */
#ifndef CALYNDA_WII_BUILD
int main(void)
{
    printf("=== Calynda Counter GUI (host mode) ===\n");
    printf("On Wii, this renders a full GUI with pointer tracking.\n");
    printf("Host mode: running one frame for verification.\n\n");

    /* Create signals */
    sig_count = solid_create_signal((CalyndaRtWord)0);

    /* Build GUI tree */
    SolidGuiWindow *root = build_counter_component();
    (void)root;

    /* Create reactive effect */
    SolidEffect *eff = solid_create_effect(counter_display_effect, NULL);

    /* Simulate a few button clicks */
    printf("Initial count: %d\n", (int)solid_signal_get(sig_count));

    solid_signal_set(sig_count, (CalyndaRtWord)1);
    printf("After increment: %d\n", (int)solid_signal_get(sig_count));

    solid_signal_set(sig_count, (CalyndaRtWord)5);
    printf("Set to 5: %d\n", (int)solid_signal_get(sig_count));

    solid_signal_set(sig_count, (CalyndaRtWord)0);
    printf("After reset: %d\n", (int)solid_signal_get(sig_count));

    /* Test pointer query */
    int px, py;
    float pa;
    bool pv;
    solid_bridge_pointer_get_position(0, &px, &py, &pa, &pv);
    printf("Pointer: x=%d y=%d angle=%.1f valid=%d\n", px, py, pa, pv);

    solid_effect_dispose(eff);
    solid_signal_dispose(sig_count);

    printf("\n=== Host verification complete ===\n");
    return 0;
}
#endif
