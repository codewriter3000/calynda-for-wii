/*
 * counter_gui.c — Minimal reactive counter with Wii remote pointer.
 *
 * This is the C output that the Calynda JSX compiler would generate
 * from counter.cal. It uses the Solite/Wii reactive runtime + bridge.
 *
 * Layout (640×480):
 *   - Dark background
 *   - "Count: N" text (reactive, updates on signal change)
 *   - [+] button  — increments counter
 *   - [-] button  — decrements counter
 *   - [Reset] button — resets to 0
 *   - Wii remote pointer cursor (small crosshair image, tracks IR)
 */

#include "solite_signal.h"
#include "solite_effect.h"
#include "solite_gui_bridge.h"
#include "calynda_runtime.h"

#include <stdio.h>
#include <string.h>

/* ════════════════════════════════════════════════════════════════════
 *  Signals
 * ════════════════════════════════════════════════════════════════════ */

static SoliteSignal *sig_count = NULL;   /* int counter value     */

/* ════════════════════════════════════════════════════════════════════
 *  GUI handles
 * ════════════════════════════════════════════════════════════════════ */

static SoliteGuiWindow *root_window   = NULL;

/* Background */
static SoliteGuiImage  *bg_image      = NULL;

/* Counter display */
static SoliteGuiText   *count_label   = NULL;

/* Buttons */
static SoliteGuiButton *btn_inc       = NULL;
static SoliteGuiText   *btn_inc_lbl   = NULL;
static SoliteGuiButton *btn_dec       = NULL;
static SoliteGuiText   *btn_dec_lbl   = NULL;
static SoliteGuiButton *btn_reset     = NULL;
static SoliteGuiText   *btn_reset_lbl = NULL;

/* Pointer cursor */
static SoliteGuiImage  *cursor_img    = NULL;

/* ════════════════════════════════════════════════════════════════════
 *  Reactive effect: update counter text when sig_count changes
 * ════════════════════════════════════════════════════════════════════ */

static void counter_display_effect(void *ctx)
{
    (void)ctx;
    CalyndaRtWord val = solite_signal_get(sig_count);
    int n = (int)val;

    /* Format "Count: N" into a stack buffer */
    char buf[64];
    snprintf(buf, sizeof(buf), "Count: %d", n);

    solite_bridge_text_set_text(count_label, buf);
}

/* ════════════════════════════════════════════════════════════════════
 *  Build the GUI tree (what JSX emit would generate)
 * ════════════════════════════════════════════════════════════════════ */

static SoliteGuiWindow *build_counter_component(void)
{
    /* ── Root window (640×480, full screen) ── */
    root_window = solite_bridge_window_new(640, 480);

    /* ── Background: dark grey rectangle ── */
    bg_image = solite_bridge_image_new_color(640, 480, 0x2D, 0x2D, 0x2D, 0xFF);
    solite_bridge_window_append(root_window,
        solite_bridge_image_as_element(bg_image));

    /* ── Title text ── */
    SoliteGuiText *title = solite_bridge_text_new("Calynda Counter", 30,
                                                 0x00, 0xCC, 0xFF, 0xFF);
    solite_bridge_element_set_position(
        solite_bridge_text_as_element(title), 200, 40);
    solite_bridge_window_append(root_window,
        solite_bridge_text_as_element(title));

    /* ── Counter value label ── */
    count_label = solite_bridge_text_new("Count: 0", 36,
                                         0xFF, 0xFF, 0xFF, 0xFF);
    solite_bridge_element_set_position(
        solite_bridge_text_as_element(count_label), 230, 140);
    solite_bridge_window_append(root_window,
        solite_bridge_text_as_element(count_label));

    /* ── [ + ] button ── */
    btn_inc = solite_bridge_button_new(120, 60);
    solite_bridge_element_set_position(
        solite_bridge_button_as_element(btn_inc), 140, 250);
    btn_inc_lbl = solite_bridge_text_new("+", 28, 0xFF, 0xFF, 0xFF, 0xFF);
    solite_bridge_button_set_label(btn_inc, btn_inc_lbl);
    solite_bridge_button_set_trigger(btn_inc);
    solite_bridge_window_append(root_window,
        solite_bridge_button_as_element(btn_inc));

    /* ── [ - ] button ── */
    btn_dec = solite_bridge_button_new(120, 60);
    solite_bridge_element_set_position(
        solite_bridge_button_as_element(btn_dec), 260, 250);
    btn_dec_lbl = solite_bridge_text_new("-", 28, 0xFF, 0xFF, 0xFF, 0xFF);
    solite_bridge_button_set_label(btn_dec, btn_dec_lbl);
    solite_bridge_button_set_trigger(btn_dec);
    solite_bridge_window_append(root_window,
        solite_bridge_button_as_element(btn_dec));

    /* ── [ Reset ] button ── */
    btn_reset = solite_bridge_button_new(120, 60);
    solite_bridge_element_set_position(
        solite_bridge_button_as_element(btn_reset), 380, 250);
    btn_reset_lbl = solite_bridge_text_new("Reset", 22, 0xFF, 0xFF, 0xFF, 0xFF);
    solite_bridge_button_set_label(btn_reset, btn_reset_lbl);
    solite_bridge_button_set_trigger(btn_reset);
    solite_bridge_window_append(root_window,
        solite_bridge_button_as_element(btn_reset));

    /* ── Pointer cursor (small white crosshair square) ── */
    cursor_img = solite_bridge_image_new_color(48, 48, 0xFF, 0xFF, 0xFF, 0xCC);
    solite_bridge_element_set_position(
        solite_bridge_image_as_element(cursor_img), 320, 240);
    /* Cursor is appended LAST so it draws on top of everything */
    solite_bridge_window_append(root_window,
        solite_bridge_image_as_element(cursor_img));

    return root_window;
}

/* ════════════════════════════════════════════════════════════════════
 *  Per-frame: poll buttons & update pointer position
 * ════════════════════════════════════════════════════════════════════ */

static void frame_poll(void)
{
    /* ── Button click checks (poll-based) ── */
    if (solite_bridge_button_was_clicked(btn_inc)) {
        CalyndaRtWord cur = solite_signal_get(sig_count);
        solite_signal_set(sig_count, cur + 1);
    }
    if (solite_bridge_button_was_clicked(btn_dec)) {
        CalyndaRtWord cur = solite_signal_get(sig_count);
        solite_signal_set(sig_count, cur > 0 ? cur - 1 : 0);
    }
    if (solite_bridge_button_was_clicked(btn_reset)) {
        solite_signal_set(sig_count, (CalyndaRtWord)0);
    }

    /* ── Update pointer cursor position from Wii remote IR ── */
    int px = 0, py = 0;
    float angle = 0.0f;
    bool valid = false;

    solite_bridge_pointer_get_position(0, &px, &py, &angle, &valid);

    SoliteGuiElement *cursor_el = solite_bridge_image_as_element(cursor_img);

    if (valid) {
        /* Center the 48×48 cursor image on the pointer position */
        solite_bridge_element_set_position(cursor_el, px - 24, py - 24);
        solite_bridge_element_set_angle(cursor_el, angle);
        solite_bridge_element_set_visible(cursor_el, true);
    } else {
        solite_bridge_element_set_visible(cursor_el, false);
    }
}

/* ════════════════════════════════════════════════════════════════════
 *  Main loop (replaces solite_mount for this standalone app)
 * ════════════════════════════════════════════════════════════════════ */

static bool app_running = true;

static void counter_main_loop(SoliteGuiWindow *root)
{
    solite_bridge_video_init();
    solite_bridge_input_init();

    while (app_running) {
        /* 1. Scan input */
        solite_bridge_input_scan();

        /* 2. Poll GUI buttons & update pointer */
        frame_poll();

        /* 3. Flush reactive effects (counter label update, etc.) */
        solite_effect_flush_pending();

        /* 4. Update GUI tree with input (hit testing) */
        solite_bridge_window_update(root);

        /* 5. Draw */
        solite_bridge_draw_start();
        solite_bridge_window_draw(root);
        solite_bridge_draw_end();
    }
}

/* ════════════════════════════════════════════════════════════════════
 *  Entry point (called from calynda boot or standalone main)
 * ════════════════════════════════════════════════════════════════════ */

void counter_app_start(void)
{
    /* Create signals */
    sig_count = solite_create_signal((CalyndaRtWord)0);

    /* Build GUI tree */
    SoliteGuiWindow *root = build_counter_component();

    /* Create reactive effect: update label whenever sig_count changes */
    solite_create_effect(counter_display_effect, NULL);

    /* Enter main loop */
    counter_main_loop(root);

    /* Cleanup */
    solite_signal_dispose(sig_count);
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
    sig_count = solite_create_signal((CalyndaRtWord)0);

    /* Build GUI tree */
    SoliteGuiWindow *root = build_counter_component();
    (void)root;

    /* Create reactive effect */
    SoliteEffect *eff = solite_create_effect(counter_display_effect, NULL);

    /* Simulate a few button clicks */
    printf("Initial count: %d\n", (int)solite_signal_get(sig_count));

    solite_signal_set(sig_count, (CalyndaRtWord)1);
    printf("After increment: %d\n", (int)solite_signal_get(sig_count));

    solite_signal_set(sig_count, (CalyndaRtWord)5);
    printf("Set to 5: %d\n", (int)solite_signal_get(sig_count));

    solite_signal_set(sig_count, (CalyndaRtWord)0);
    printf("After reset: %d\n", (int)solite_signal_get(sig_count));

    /* Test pointer query */
    int px, py;
    float pa;
    bool pv;
    solite_bridge_pointer_get_position(0, &px, &py, &pa, &pv);
    printf("Pointer: x=%d y=%d angle=%.1f valid=%d\n", px, py, pa, pv);

    solite_effect_dispose(eff);
    solite_signal_dispose(sig_count);

    printf("\n=== Host verification complete ===\n");
    return 0;
}
#endif
