#ifndef SOLITE_GUI_BRIDGE_H
#define SOLITE_GUI_BRIDGE_H

/*
 * solite_gui_bridge.h — C-linkage bridge to libwiigui's C++ API.
 *
 * All libwiigui interaction goes through these opaque handles and
 * functions so the reactive runtime (pure C) never touches C++ directly.
 *
 * The bridge implementation (solid_gui_bridge.cpp) wraps libwiigui
 * classes with extern "C" functions.
 */

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---- Opaque handle types ---- */

/* These map 1:1 to libwiigui classes on the C++ side. */
typedef struct SoliteGuiElement   SoliteGuiElement;
typedef struct SoliteGuiWindow    SoliteGuiWindow;
typedef struct SoliteGuiText      SoliteGuiText;
typedef struct SoliteGuiImage     SoliteGuiImage;
typedef struct SoliteGuiImageData SoliteGuiImageData;
typedef struct SoliteGuiButton    SoliteGuiButton;
typedef struct SoliteGuiSound     SoliteGuiSound;
typedef struct SoliteGuiTrigger   SoliteGuiTrigger;

/* ---- Callback type for button click handlers ---- */
/*
 * buttons:      array of string names for buttons pressed (e.g. "A", "B")
 * button_count: number of strings in the array
 * context:      user data pointer registered with the handler
 */
typedef void (*SoliteClickHandler)(const char **buttons, int button_count, void *context);

/* ---- Video / draw lifecycle ---- */

void solite_bridge_video_init(void);
void solite_bridge_draw_start(void);
void solite_bridge_draw_end(void);

/* ---- Input ---- */

void solite_bridge_input_init(void);
void solite_bridge_input_scan(void);

/* ---- Window ---- */

SoliteGuiWindow *solite_bridge_window_new(int width, int height);
void solite_bridge_window_append(SoliteGuiWindow *win, SoliteGuiElement *child);
void solite_bridge_window_remove(SoliteGuiWindow *win, SoliteGuiElement *child);
void solite_bridge_window_update(SoliteGuiWindow *win);
void solite_bridge_window_draw(SoliteGuiWindow *win);

/* ---- Text ---- */

SoliteGuiText *solite_bridge_text_new(const char *text, int font_size,
                                    uint8_t r, uint8_t g, uint8_t b, uint8_t a);
void solite_bridge_text_set_text(SoliteGuiText *txt, const char *text);
void solite_bridge_text_set_font_size(SoliteGuiText *txt, int size);
void solite_bridge_text_set_max_width(SoliteGuiText *txt, int width);

/* ---- Image ---- */

SoliteGuiImageData *solite_bridge_imagedata_new(const uint8_t *png_data, size_t png_size);
SoliteGuiImage     *solite_bridge_image_new(SoliteGuiImageData *data);
SoliteGuiImage     *solite_bridge_image_new_color(int w, int h,
                                                uint8_t r, uint8_t g, uint8_t b, uint8_t a);

/* ---- Button ---- */

SoliteGuiButton *solite_bridge_button_new(int width, int height);
void solite_bridge_button_set_image(SoliteGuiButton *btn, SoliteGuiImage *img);
void solite_bridge_button_set_label(SoliteGuiButton *btn, SoliteGuiText *txt);
void solite_bridge_button_set_trigger(SoliteGuiButton *btn);
void solite_bridge_button_set_click_handler(SoliteGuiButton *btn,
                                           SoliteClickHandler handler, void *context);
bool solite_bridge_button_was_clicked(SoliteGuiButton *btn);

/*
 * Poll all registered buttons for clicks.
 * When a button is in the CLICKED state, builds an array of pressed-button
 * names from WPAD/PAD state and dispatches the registered handler.
 * Call once per frame, after window_update().
 */
void solite_bridge_poll_button_clicks(void);

/*
 * Query which buttons are currently held/down on the given channel.
 * Returns an array of const string pointers (static storage) and sets
 * *out_count.  The caller must NOT free the strings.
 */
const char **solite_bridge_get_pressed_buttons(int channel, int *out_count);

/* ---- Sound ---- */

SoliteGuiSound *solite_bridge_sound_new(const uint8_t *ogg_data, size_t ogg_size);
void solite_bridge_sound_play(SoliteGuiSound *snd);
void solite_bridge_sound_stop(SoliteGuiSound *snd);

/* ---- Element (base) ---- */

void solite_bridge_element_set_position(SoliteGuiElement *el, int x, int y);
void solite_bridge_element_set_alignment_top_left(SoliteGuiElement *el);
void solite_bridge_element_set_alpha(SoliteGuiElement *el, int alpha);
void solite_bridge_element_set_visible(SoliteGuiElement *el, bool visible);
void solite_bridge_element_set_scale(SoliteGuiElement *el, float scale);

/*
 * Style-system bridge functions.
 * Called by solid_style.c apply functions.
 */
void solite_bridge_element_set_size_w(SoliteGuiElement *el, int width);
void solite_bridge_element_set_size_h(SoliteGuiElement *el, int height);
void solite_bridge_element_set_color(SoliteGuiElement *el,
                                    uint8_t r, uint8_t g, uint8_t b, uint8_t a);
void solite_bridge_element_set_background_color(SoliteGuiElement *el,
                                               uint8_t r, uint8_t g, uint8_t b, uint8_t a);

/* ---- Element angle/rotation ---- */

void solite_bridge_element_set_angle(SoliteGuiElement *el, float angle_deg);

/* ---- Pointer / cursor ---- */

/*
 * Query the Wii remote IR pointer position for the given channel (0–3).
 * x/y are in screen coordinates (640×480). valid is false when the
 * pointer is offscreen.
 */
void solite_bridge_pointer_get_position(int channel,
                                       int *out_x, int *out_y,
                                       float *out_angle,
                                       bool *out_valid);

/* ---- Casting helpers ---- */

/*
 * Upcast widget types to base SoliteGuiElement for use with style system
 * and window append/remove.
 */
SoliteGuiElement *solite_bridge_text_as_element(SoliteGuiText *txt);
SoliteGuiElement *solite_bridge_image_as_element(SoliteGuiImage *img);
SoliteGuiElement *solite_bridge_button_as_element(SoliteGuiButton *btn);
SoliteGuiElement *solite_bridge_window_as_element(SoliteGuiWindow *win);

#ifdef __cplusplus
}
#endif

#endif /* SOLITE_GUI_BRIDGE_H */
