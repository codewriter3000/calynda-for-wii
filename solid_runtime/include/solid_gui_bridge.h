#ifndef SOLID_GUI_BRIDGE_H
#define SOLID_GUI_BRIDGE_H

/*
 * solid_gui_bridge.h — C-linkage bridge to libwiigui's C++ API.
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
typedef struct SolidGuiElement   SolidGuiElement;
typedef struct SolidGuiWindow    SolidGuiWindow;
typedef struct SolidGuiText      SolidGuiText;
typedef struct SolidGuiImage     SolidGuiImage;
typedef struct SolidGuiImageData SolidGuiImageData;
typedef struct SolidGuiButton    SolidGuiButton;
typedef struct SolidGuiSound     SolidGuiSound;
typedef struct SolidGuiTrigger   SolidGuiTrigger;

/* ---- Callback type for button click handlers ---- */
typedef void (*SolidClickHandler)(void *context);

/* ---- Video / draw lifecycle ---- */

void solid_bridge_video_init(void);
void solid_bridge_draw_start(void);
void solid_bridge_draw_end(void);

/* ---- Input ---- */

void solid_bridge_input_init(void);
void solid_bridge_input_scan(void);

/* ---- Window ---- */

SolidGuiWindow *solid_bridge_window_new(int width, int height);
void solid_bridge_window_append(SolidGuiWindow *win, SolidGuiElement *child);
void solid_bridge_window_remove(SolidGuiWindow *win, SolidGuiElement *child);
void solid_bridge_window_update(SolidGuiWindow *win);
void solid_bridge_window_draw(SolidGuiWindow *win);

/* ---- Text ---- */

SolidGuiText *solid_bridge_text_new(const char *text, int font_size,
                                    uint8_t r, uint8_t g, uint8_t b, uint8_t a);
void solid_bridge_text_set_text(SolidGuiText *txt, const char *text);
void solid_bridge_text_set_font_size(SolidGuiText *txt, int size);
void solid_bridge_text_set_max_width(SolidGuiText *txt, int width);

/* ---- Image ---- */

SolidGuiImageData *solid_bridge_imagedata_new(const uint8_t *png_data, size_t png_size);
SolidGuiImage     *solid_bridge_image_new(SolidGuiImageData *data);
SolidGuiImage     *solid_bridge_image_new_color(int w, int h,
                                                uint8_t r, uint8_t g, uint8_t b, uint8_t a);

/* ---- Button ---- */

SolidGuiButton *solid_bridge_button_new(int width, int height);
void solid_bridge_button_set_image(SolidGuiButton *btn, SolidGuiImage *img);
void solid_bridge_button_set_label(SolidGuiButton *btn, SolidGuiText *txt);
void solid_bridge_button_set_trigger(SolidGuiButton *btn);
void solid_bridge_button_set_click_handler(SolidGuiButton *btn,
                                           SolidClickHandler handler, void *context);
bool solid_bridge_button_was_clicked(SolidGuiButton *btn);

/* ---- Sound ---- */

SolidGuiSound *solid_bridge_sound_new(const uint8_t *ogg_data, size_t ogg_size);
void solid_bridge_sound_play(SolidGuiSound *snd);
void solid_bridge_sound_stop(SolidGuiSound *snd);

/* ---- Element (base) ---- */

void solid_bridge_element_set_position(SolidGuiElement *el, int x, int y);
void solid_bridge_element_set_alpha(SolidGuiElement *el, int alpha);
void solid_bridge_element_set_visible(SolidGuiElement *el, bool visible);
void solid_bridge_element_set_scale(SolidGuiElement *el, float scale);

/*
 * Style-system bridge functions.
 * Called by solid_style.c apply functions.
 */
void solid_bridge_element_set_size_w(SolidGuiElement *el, int width);
void solid_bridge_element_set_size_h(SolidGuiElement *el, int height);
void solid_bridge_element_set_color(SolidGuiElement *el,
                                    uint8_t r, uint8_t g, uint8_t b, uint8_t a);
void solid_bridge_element_set_background_color(SolidGuiElement *el,
                                               uint8_t r, uint8_t g, uint8_t b, uint8_t a);

/* ---- Casting helpers ---- */

/*
 * Upcast widget types to base SolidGuiElement for use with style system
 * and window append/remove.
 */
SolidGuiElement *solid_bridge_text_as_element(SolidGuiText *txt);
SolidGuiElement *solid_bridge_image_as_element(SolidGuiImage *img);
SolidGuiElement *solid_bridge_button_as_element(SolidGuiButton *btn);
SolidGuiElement *solid_bridge_window_as_element(SolidGuiWindow *win);

#ifdef __cplusplus
}
#endif

#endif /* SOLID_GUI_BRIDGE_H */
