/*
 * solid_gui_bridge.cpp — C-linkage bridge to libwiigui's C++ classes.
 *
 * Each function casts the opaque handle to the real libwiigui class,
 * calls the method, and returns. This is the ONLY file that includes
 * the C++ libwiigui headers.
 */

#ifdef SOLITE_HAS_LIBWIIGUI
/* Real libwiigui build (Wii target) */
#include "gui.h"
#include "input.h"
#else
/* Host/stub build — provide minimal stubs so the C runtime compiles */
#endif

extern "C" {
#include "solite_gui_bridge.h"
}

#include <cstdlib>
#include <cstdio>
#include <cstring>

/* ================================================================== */
/*  Helper: reinterpret casts between opaque handles and real classes  */
/* ================================================================== */

#ifdef SOLITE_HAS_LIBWIIGUI

#define AS_ELEMENT(p) reinterpret_cast<GuiElement *>(p)
#define AS_WINDOW(p)  reinterpret_cast<GuiWindow *>(p)
#define AS_TEXT(p)    reinterpret_cast<GuiText *>(p)
#define AS_IMAGE(p)   reinterpret_cast<GuiImage *>(p)
#define AS_IMGDATA(p) reinterpret_cast<GuiImageData *>(p)
#define AS_BUTTON(p)  reinterpret_cast<GuiButton *>(p)
#define AS_SOUND(p)   reinterpret_cast<GuiSound *>(p)

#define TO_HANDLE(cls, p) reinterpret_cast<cls *>(p)

/* Per-button click handler storage */
#define MAX_REGISTERED_BUTTONS 32
struct ButtonClickEntry {
    SoliteGuiButton   *btn;
    SoliteClickHandler handler;
    void             *context;
};
static ButtonClickEntry registered_buttons[MAX_REGISTERED_BUTTONS];
static int              registered_button_count = 0;

static GuiTrigger trigger_a;
static bool trigger_initialized = false;

/* ---- Video ---- */

extern "C" void solite_bridge_video_init(void) {
    InitVideo();
    SetupPads();
    InitFreeType((uint8_t *)font_ttf, font_ttf_size);
    if (!trigger_initialized) {
        trigger_a.SetSimpleTrigger(-1, WPAD_BUTTON_A | WPAD_CLASSIC_BUTTON_A, PAD_BUTTON_A);
        trigger_initialized = true;
    }
}

extern "C" void solite_bridge_draw_start(void) {
    ResetVideo_Menu();
}

extern "C" void solite_bridge_draw_end(void) {
    Menu_Render();
}

/* ---- Input ---- */

extern "C" void solite_bridge_input_init(void) {
    /* SetupPads() in video_init already calls WPAD_Init/PAD_Init
       and sets userInput[i].wpad = WPAD_Data(i). Nothing more needed. */
}

extern "C" void solite_bridge_input_scan(void) {
    UpdatePads();
}

/* ---- Window ---- */

extern "C" SoliteGuiWindow *solite_bridge_window_new(int w, int h) {
    return TO_HANDLE(SoliteGuiWindow, new GuiWindow(w, h));
}

extern "C" void solite_bridge_window_append(SoliteGuiWindow *win, SoliteGuiElement *child) {
    if (win && child) AS_WINDOW(win)->Append(AS_ELEMENT(child));
}

extern "C" void solite_bridge_window_remove(SoliteGuiWindow *win, SoliteGuiElement *child) {
    if (win && child) AS_WINDOW(win)->Remove(AS_ELEMENT(child));
}

extern "C" void solite_bridge_window_update(SoliteGuiWindow *win) {
    if (!win) return;
    for (int i = 0; i < 4; i++)
        AS_WINDOW(win)->Update(&userInput[i]);
}

extern "C" void solite_bridge_window_draw(SoliteGuiWindow *win) {
    if (win) AS_WINDOW(win)->Draw();
}

/* ---- Text ---- */

extern "C" SoliteGuiText *solite_bridge_text_new(const char *text, int size,
                                                uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    GXColor c = {r, g, b, a};
    return TO_HANDLE(SoliteGuiText, new GuiText(text, size, c));
}

extern "C" void solite_bridge_text_set_text(SoliteGuiText *txt, const char *text) {
    if (txt) AS_TEXT(txt)->SetText(text);
}

extern "C" void solite_bridge_text_set_font_size(SoliteGuiText *txt, int size) {
    if (txt) AS_TEXT(txt)->SetFontSize(size);
}

extern "C" void solite_bridge_text_set_max_width(SoliteGuiText *txt, int width) {
    if (txt) AS_TEXT(txt)->SetMaxWidth(width);
}

/* ---- Image ---- */

extern "C" SoliteGuiImageData *solite_bridge_imagedata_new(const uint8_t *data, size_t size) {
    return TO_HANDLE(SoliteGuiImageData, new GuiImageData(data, size));
}

extern "C" SoliteGuiImage *solite_bridge_image_new(SoliteGuiImageData *data) {
    return TO_HANDLE(SoliteGuiImage, new GuiImage(AS_IMGDATA(data)));
}

extern "C" SoliteGuiImage *solite_bridge_image_new_color(int w, int h,
                                                        uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    GXColor c = {r, g, b, a};
    return TO_HANDLE(SoliteGuiImage, new GuiImage(w, h, c));
}

/* ---- Button ---- */

extern "C" SoliteGuiButton *solite_bridge_button_new(int w, int h) {
    GuiButton *btn = new GuiButton(w, h);
    btn->SetTrigger(&trigger_a);
    btn->SetSelectable(true);
    btn->SetClickable(true);
    return TO_HANDLE(SoliteGuiButton, btn);
}

extern "C" void solite_bridge_button_set_image(SoliteGuiButton *btn, SoliteGuiImage *img) {
    if (btn) AS_BUTTON(btn)->SetImage(AS_IMAGE(img));
}

extern "C" void solite_bridge_button_set_label(SoliteGuiButton *btn, SoliteGuiText *txt) {
    if (btn) AS_BUTTON(btn)->SetLabel(AS_TEXT(txt));
}

extern "C" void solite_bridge_button_set_trigger(SoliteGuiButton *btn) {
    if (btn) AS_BUTTON(btn)->SetTrigger(&trigger_a);
}

extern "C" void solite_bridge_button_set_click_handler(SoliteGuiButton *btn,
                                                       SoliteClickHandler handler, void *ctx) {
    if (!btn || !handler) return;
    if (registered_button_count >= MAX_REGISTERED_BUTTONS) return;
    ButtonClickEntry *e = &registered_buttons[registered_button_count++];
    e->btn     = btn;
    e->handler = handler;
    e->context = ctx;
}

extern "C" bool solite_bridge_button_was_clicked(SoliteGuiButton *btn) {
    if (!btn) return false;
    GuiButton *b = AS_BUTTON(btn);
    if (b->GetState() == STATE::CLICKED) {
        b->ResetState();
        return true;
    }
    return false;
}

/* ---- Button names from WPAD/PAD bitmask ---- */

static const char *wpad_button_names[] = {
    "2", "1", "B", "A", "Minus", NULL, NULL, "Home",
    "Left", "Right", "Down", "Up", "Plus"
};
#define WPAD_BUTTON_BITS 13

extern "C" const char **solite_bridge_get_pressed_buttons(int channel, int *out_count) {
    static const char *result[16];
    int count = 0;
    if (channel < 0 || channel > 3) {
        if (out_count) *out_count = 0;
        return result;
    }
    uint32_t btns = WPAD_ButtonsDown(channel);
    for (int i = 0; i < WPAD_BUTTON_BITS && count < 15; i++) {
        if ((btns & (1u << i)) && wpad_button_names[i]) {
            result[count++] = wpad_button_names[i];
        }
    }
    /* Also check GameCube pad */
    uint16_t pad = PAD_ButtonsDown(channel);
    if ((pad & PAD_BUTTON_A)     && count < 15) result[count++] = "A";
    if ((pad & PAD_BUTTON_B)     && count < 15) result[count++] = "B";
    if ((pad & PAD_BUTTON_X)     && count < 15) result[count++] = "X";
    if ((pad & PAD_BUTTON_Y)     && count < 15) result[count++] = "Y";
    if ((pad & PAD_BUTTON_START) && count < 15) result[count++] = "Start";
    result[count] = NULL;
    if (out_count) *out_count = count;
    return result;
}

extern "C" void solite_bridge_poll_button_clicks(void) {
    for (int i = 0; i < registered_button_count; i++) {
        ButtonClickEntry *e = &registered_buttons[i];
        GuiButton *b = AS_BUTTON(e->btn);
        if (b->GetState() == STATE::CLICKED) {
            b->ResetState();
            /* Determine which channel triggered the click.
               Check all 4 channels for any buttons down. */
            const char **btns = NULL;
            int btn_count = 0;
            for (int ch = 0; ch < 4; ch++) {
                btns = solite_bridge_get_pressed_buttons(ch, &btn_count);
                if (btn_count > 0) break;
            }
            /* If no specific button detected, report "A" as default
               since the trigger was set to A */
            static const char *default_btn[] = {"A", NULL};
            if (btn_count == 0) {
                btns = default_btn;
                btn_count = 1;
            }
            e->handler(btns, btn_count, e->context);
        }
    }
}

/* ---- Sound ---- */

extern "C" SoliteGuiSound *solite_bridge_sound_new(const uint8_t *data, size_t size) {
    return TO_HANDLE(SoliteGuiSound, new GuiSound(data, size, SOUND::OGG));
}

extern "C" void solite_bridge_sound_play(SoliteGuiSound *snd) {
    if (snd) AS_SOUND(snd)->Play();
}

extern "C" void solite_bridge_sound_stop(SoliteGuiSound *snd) {
    if (snd) AS_SOUND(snd)->Stop();
}

/* ---- Element (base) ---- */

extern "C" void solite_bridge_element_set_position(SoliteGuiElement *el, int x, int y) {
    if (el) AS_ELEMENT(el)->SetPosition(x, y);
}

extern "C" void solite_bridge_element_set_alignment_top_left(SoliteGuiElement *el) {
    if (el) AS_ELEMENT(el)->SetAlignment(ALIGN_H::LEFT, ALIGN_V::TOP);
}

extern "C" void solite_bridge_element_set_alpha(SoliteGuiElement *el, int alpha) {
    if (el) AS_ELEMENT(el)->SetAlpha(alpha);
}

extern "C" void solite_bridge_element_set_visible(SoliteGuiElement *el, bool visible) {
    if (el) AS_ELEMENT(el)->SetVisible(visible);
}

extern "C" void solite_bridge_element_set_scale(SoliteGuiElement *el, float scale) {
    if (el) AS_ELEMENT(el)->SetScale(scale);
}

extern "C" void solite_bridge_element_set_size_w(SoliteGuiElement *el, int width) {
    if (!el) return;
    GuiElement *e = AS_ELEMENT(el);
    e->SetSize(width, e->GetHeight());
}

extern "C" void solite_bridge_element_set_size_h(SoliteGuiElement *el, int height) {
    if (!el) return;
    GuiElement *e = AS_ELEMENT(el);
    e->SetSize(e->GetWidth(), height);
}

extern "C" void solite_bridge_element_set_color(SoliteGuiElement *el,
                                                uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    if (!el) return;
    /* Color applies to GuiText — check by attempting dynamic cast isn't
       available without RTTI, so we use SetColor if the element is a text.
       The bridge user (code emitter) knows the concrete type. */
    GuiText *txt = reinterpret_cast<GuiText *>(el);
    GXColor c = {r, g, b, a};
    txt->SetColor(c);
}

extern "C" void solite_bridge_element_set_background_color(SoliteGuiElement *el,
                                                           uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    /* Background color creates a colored rectangle behind the element.
       Implemented by wrapping in a GuiImage with the given color.
       For now, this is a no-op placeholder — the emitter should create
       a GuiImage background separately and Append it before the element. */
    (void)el; (void)r; (void)g; (void)b; (void)a;
}

/* ---- Element angle/rotation ---- */

extern "C" void solite_bridge_element_set_angle(SoliteGuiElement *el, float angle_deg) {
    /* GuiElement does not support SetAngle — no-op */
    (void)el; (void)angle_deg;
}

/* ---- Pointer / cursor ---- */

extern "C" void solite_bridge_pointer_get_position(int channel,
                                                   int *out_x, int *out_y,
                                                   float *out_angle,
                                                   bool *out_valid) {
    if (channel < 0 || channel > 3) {
        if (out_valid) *out_valid = false;
        return;
    }
    ir_t ir;
    WPAD_IR(channel, &ir);
    if (out_x) *out_x = (int)ir.x;
    if (out_y) *out_y = (int)ir.y;
    if (out_angle) *out_angle = ir.angle;
    if (out_valid) *out_valid = (ir.valid != 0);
}

/* ---- Casting helpers ---- */

extern "C" SoliteGuiElement *solite_bridge_text_as_element(SoliteGuiText *txt) {
    return reinterpret_cast<SoliteGuiElement *>(static_cast<GuiElement *>(AS_TEXT(txt)));
}

extern "C" SoliteGuiElement *solite_bridge_image_as_element(SoliteGuiImage *img) {
    return reinterpret_cast<SoliteGuiElement *>(static_cast<GuiElement *>(AS_IMAGE(img)));
}

extern "C" SoliteGuiElement *solite_bridge_button_as_element(SoliteGuiButton *btn) {
    return reinterpret_cast<SoliteGuiElement *>(static_cast<GuiElement *>(AS_BUTTON(btn)));
}

extern "C" SoliteGuiElement *solite_bridge_window_as_element(SoliteGuiWindow *win) {
    return reinterpret_cast<SoliteGuiElement *>(static_cast<GuiElement *>(AS_WINDOW(win)));
}

#else /* !SOLITE_HAS_LIBWIIGUI — Host stubs for testing */

#include <cstdio>

extern "C" void solite_bridge_video_init(void) { }
extern "C" void solite_bridge_draw_start(void) { }
extern "C" void solite_bridge_draw_end(void) { }
extern "C" void solite_bridge_input_init(void) { }
extern "C" void solite_bridge_input_scan(void) { }

extern "C" SoliteGuiWindow *solite_bridge_window_new(int w, int h) {
    (void)w; (void)h;
    return (SoliteGuiWindow *)calloc(1, 64);
}
extern "C" void solite_bridge_window_append(SoliteGuiWindow *w, SoliteGuiElement *c) { (void)w; (void)c; }
extern "C" void solite_bridge_window_remove(SoliteGuiWindow *w, SoliteGuiElement *c) { (void)w; (void)c; }
extern "C" void solite_bridge_window_update(SoliteGuiWindow *w) { (void)w; }
extern "C" void solite_bridge_window_draw(SoliteGuiWindow *w) { (void)w; }

extern "C" SoliteGuiText *solite_bridge_text_new(const char *t, int s,
    uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    (void)t; (void)s; (void)r; (void)g; (void)b; (void)a;
    return (SoliteGuiText *)calloc(1, 64);
}
extern "C" void solite_bridge_text_set_text(SoliteGuiText *t, const char *s) { (void)t; (void)s; }
extern "C" void solite_bridge_text_set_font_size(SoliteGuiText *t, int s) { (void)t; (void)s; }
extern "C" void solite_bridge_text_set_max_width(SoliteGuiText *t, int w) { (void)t; (void)w; }

extern "C" SoliteGuiImageData *solite_bridge_imagedata_new(const uint8_t *d, size_t s) {
    (void)d; (void)s;
    return (SoliteGuiImageData *)calloc(1, 64);
}
extern "C" SoliteGuiImage *solite_bridge_image_new(SoliteGuiImageData *d) {
    (void)d;
    return (SoliteGuiImage *)calloc(1, 64);
}
extern "C" SoliteGuiImage *solite_bridge_image_new_color(int w, int h,
    uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    (void)w; (void)h; (void)r; (void)g; (void)b; (void)a;
    return (SoliteGuiImage *)calloc(1, 64);
}

extern "C" SoliteGuiButton *solite_bridge_button_new(int w, int h) {
    (void)w; (void)h;
    return (SoliteGuiButton *)calloc(1, 64);
}
extern "C" void solite_bridge_button_set_image(SoliteGuiButton *b, SoliteGuiImage *i) { (void)b; (void)i; }
extern "C" void solite_bridge_button_set_label(SoliteGuiButton *b, SoliteGuiText *t) { (void)b; (void)t; }
extern "C" void solite_bridge_button_set_trigger(SoliteGuiButton *b) { (void)b; }
extern "C" void solite_bridge_button_set_click_handler(SoliteGuiButton *b,
    SoliteClickHandler h, void *c) { (void)b; (void)h; (void)c; }
extern "C" bool solite_bridge_button_was_clicked(SoliteGuiButton *b) { (void)b; return false; }
extern "C" const char **solite_bridge_get_pressed_buttons(int ch, int *out_count) {
    (void)ch;
    static const char *empty[] = {NULL};
    if (out_count) *out_count = 0;
    return empty;
}
extern "C" void solite_bridge_poll_button_clicks(void) { }

extern "C" SoliteGuiSound *solite_bridge_sound_new(const uint8_t *d, size_t s) {
    (void)d; (void)s;
    return (SoliteGuiSound *)calloc(1, 64);
}
extern "C" void solite_bridge_sound_play(SoliteGuiSound *s) { (void)s; }
extern "C" void solite_bridge_sound_stop(SoliteGuiSound *s) { (void)s; }

extern "C" void solite_bridge_element_set_position(SoliteGuiElement *e, int x, int y) { (void)e; (void)x; (void)y; }
extern "C" void solite_bridge_element_set_alignment_top_left(SoliteGuiElement *e) { (void)e; }
extern "C" void solite_bridge_element_set_alpha(SoliteGuiElement *e, int a) { (void)e; (void)a; }
extern "C" void solite_bridge_element_set_visible(SoliteGuiElement *e, bool v) { (void)e; (void)v; }
extern "C" void solite_bridge_element_set_scale(SoliteGuiElement *e, float s) { (void)e; (void)s; }
extern "C" void solite_bridge_element_set_size_w(SoliteGuiElement *e, int w) { (void)e; (void)w; }
extern "C" void solite_bridge_element_set_size_h(SoliteGuiElement *e, int h) { (void)e; (void)h; }
extern "C" void solite_bridge_element_set_color(SoliteGuiElement *e,
    uint8_t r, uint8_t g, uint8_t b, uint8_t a) { (void)e; (void)r; (void)g; (void)b; (void)a; }
extern "C" void solite_bridge_element_set_background_color(SoliteGuiElement *e,
    uint8_t r, uint8_t g, uint8_t b, uint8_t a) { (void)e; (void)r; (void)g; (void)b; (void)a; }

extern "C" SoliteGuiElement *solite_bridge_text_as_element(SoliteGuiText *t) { return (SoliteGuiElement *)t; }
extern "C" SoliteGuiElement *solite_bridge_image_as_element(SoliteGuiImage *i) { return (SoliteGuiElement *)i; }
extern "C" SoliteGuiElement *solite_bridge_button_as_element(SoliteGuiButton *b) { return (SoliteGuiElement *)b; }
extern "C" SoliteGuiElement *solite_bridge_window_as_element(SoliteGuiWindow *w) { return (SoliteGuiElement *)w; }

extern "C" void solite_bridge_element_set_angle(SoliteGuiElement *e, float a) { (void)e; (void)a; }
extern "C" void solite_bridge_pointer_get_position(int ch,
    int *ox, int *oy, float *oa, bool *ov) {
    /* Host stub: pointer at center, always valid */
    (void)ch;
    if (ox) *ox = 320;
    if (oy) *oy = 240;
    if (oa) *oa = 0.0f;
    if (ov) *ov = true;
}

#endif /* SOLITE_HAS_LIBWIIGUI */
