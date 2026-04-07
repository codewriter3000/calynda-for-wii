/*
 * solid_gui_bridge.cpp — C-linkage bridge to libwiigui's C++ classes.
 *
 * Each function casts the opaque handle to the real libwiigui class,
 * calls the method, and returns. This is the ONLY file that includes
 * the C++ libwiigui headers.
 */

#ifdef SOLID_HAS_LIBWIIGUI
/* Real libwiigui build (Wii target) */
#include "gui.h"
#else
/* Host/stub build — provide minimal stubs so the C runtime compiles */
#endif

extern "C" {
#include "solid_gui_bridge.h"
}

#include <cstdlib>
#include <cstdio>
#include <cstring>

/* ================================================================== */
/*  Helper: reinterpret casts between opaque handles and real classes  */
/* ================================================================== */

#ifdef SOLID_HAS_LIBWIIGUI

#define AS_ELEMENT(p) reinterpret_cast<GuiElement *>(p)
#define AS_WINDOW(p)  reinterpret_cast<GuiWindow *>(p)
#define AS_TEXT(p)    reinterpret_cast<GuiText *>(p)
#define AS_IMAGE(p)   reinterpret_cast<GuiImage *>(p)
#define AS_IMGDATA(p) reinterpret_cast<GuiImageData *>(p)
#define AS_BUTTON(p)  reinterpret_cast<GuiButton *>(p)
#define AS_SOUND(p)   reinterpret_cast<GuiSound *>(p)

#define TO_HANDLE(cls, p) reinterpret_cast<cls *>(p)

/* Per-button click handler storage */
struct ButtonClickData {
    SolidClickHandler handler;
    void *context;
};

static GuiTrigger trigger_a;
static bool trigger_initialized = false;

/* ---- Video ---- */

extern "C" void solid_bridge_video_init(void) {
    InitVideo();
    SetupPads();
    if (!trigger_initialized) {
        trigger_a.SetSimpleTrigger(-1, WPAD_BUTTON_A | WPAD_CLASSIC_BUTTON_A, PAD_BUTTON_A);
        trigger_initialized = true;
    }
}

extern "C" void solid_bridge_draw_start(void) {
    Menu_DrawStandard();
}

extern "C" void solid_bridge_draw_end(void) {
    Menu_Render();
}

/* ---- Input ---- */

extern "C" void solid_bridge_input_init(void) {
    WPAD_Init();
    PAD_Init();
}

extern "C" void solid_bridge_input_scan(void) {
    for (int i = 0; i < 4; i++)
        userInput[i].wpad->btns_d = 0;
    WPAD_ScanPads();
    PAD_ScanPads();
    for (int i = 0; i < 4; i++) {
        userInput[i].chan = i;
        WPAD_IR(i, &userInput[i].wpad->ir);
        userInput[i].pad.btns_d = PAD_ButtonsDown(i);
        userInput[i].pad.btns_u = PAD_ButtonsUp(i);
        userInput[i].pad.btns_h = PAD_ButtonsHeld(i);
        userInput[i].pad.stickX = PAD_StickX(i);
        userInput[i].pad.stickY = PAD_StickY(i);
        userInput[i].pad.substickX = PAD_SubStickX(i);
        userInput[i].pad.substickY = PAD_SubStickY(i);
    }
}

/* ---- Window ---- */

extern "C" SolidGuiWindow *solid_bridge_window_new(int w, int h) {
    return TO_HANDLE(SolidGuiWindow, new GuiWindow(w, h));
}

extern "C" void solid_bridge_window_append(SolidGuiWindow *win, SolidGuiElement *child) {
    if (win && child) AS_WINDOW(win)->Append(AS_ELEMENT(child));
}

extern "C" void solid_bridge_window_remove(SolidGuiWindow *win, SolidGuiElement *child) {
    if (win && child) AS_WINDOW(win)->Remove(AS_ELEMENT(child));
}

extern "C" void solid_bridge_window_update(SolidGuiWindow *win) {
    if (!win) return;
    for (int i = 0; i < 4; i++)
        AS_WINDOW(win)->Update(&userInput[i]);
}

extern "C" void solid_bridge_window_draw(SolidGuiWindow *win) {
    if (win) AS_WINDOW(win)->Draw();
}

/* ---- Text ---- */

extern "C" SolidGuiText *solid_bridge_text_new(const char *text, int size,
                                                uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    GXColor c = {r, g, b, a};
    return TO_HANDLE(SolidGuiText, new GuiText(text, size, c));
}

extern "C" void solid_bridge_text_set_text(SolidGuiText *txt, const char *text) {
    if (txt) AS_TEXT(txt)->SetText(text);
}

extern "C" void solid_bridge_text_set_font_size(SolidGuiText *txt, int size) {
    if (txt) AS_TEXT(txt)->SetFontSize(size);
}

extern "C" void solid_bridge_text_set_max_width(SolidGuiText *txt, int width) {
    if (txt) AS_TEXT(txt)->SetMaxWidth(width);
}

/* ---- Image ---- */

extern "C" SolidGuiImageData *solid_bridge_imagedata_new(const uint8_t *data, size_t size) {
    return TO_HANDLE(SolidGuiImageData, new GuiImageData(data, size));
}

extern "C" SolidGuiImage *solid_bridge_image_new(SolidGuiImageData *data) {
    return TO_HANDLE(SolidGuiImage, new GuiImage(AS_IMGDATA(data)));
}

extern "C" SolidGuiImage *solid_bridge_image_new_color(int w, int h,
                                                        uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    GXColor c = {r, g, b, a};
    return TO_HANDLE(SolidGuiImage, new GuiImage(w, h, c));
}

/* ---- Button ---- */

extern "C" SolidGuiButton *solid_bridge_button_new(int w, int h) {
    GuiButton *btn = new GuiButton(w, h);
    btn->SetTrigger(&trigger_a);
    btn->SetSelectable(true);
    btn->SetClickable(true);
    return TO_HANDLE(SolidGuiButton, btn);
}

extern "C" void solid_bridge_button_set_image(SolidGuiButton *btn, SolidGuiImage *img) {
    if (btn) AS_BUTTON(btn)->SetImage(AS_IMAGE(img));
}

extern "C" void solid_bridge_button_set_label(SolidGuiButton *btn, SolidGuiText *txt) {
    if (btn) AS_BUTTON(btn)->SetLabel(AS_TEXT(txt));
}

extern "C" void solid_bridge_button_set_trigger(SolidGuiButton *btn) {
    if (btn) AS_BUTTON(btn)->SetTrigger(&trigger_a);
}

extern "C" void solid_bridge_button_set_click_handler(SolidGuiButton *btn,
                                                       SolidClickHandler handler, void *ctx) {
    /* Store handler in the update callback mechanism.
       The actual click checking happens in the frame loop via
       solid_bridge_button_was_clicked(). */
    (void)btn; (void)handler; (void)ctx;
    /* For now, click detection is poll-based via was_clicked() */
}

extern "C" bool solid_bridge_button_was_clicked(SolidGuiButton *btn) {
    if (!btn) return false;
    GuiButton *b = AS_BUTTON(btn);
    if (b->GetState() == STATE::CLICKED) {
        b->ResetState();
        return true;
    }
    return false;
}

/* ---- Sound ---- */

extern "C" SolidGuiSound *solid_bridge_sound_new(const uint8_t *data, size_t size) {
    return TO_HANDLE(SolidGuiSound, new GuiSound(data, size, SOUND_OGG));
}

extern "C" void solid_bridge_sound_play(SolidGuiSound *snd) {
    if (snd) AS_SOUND(snd)->Play();
}

extern "C" void solid_bridge_sound_stop(SolidGuiSound *snd) {
    if (snd) AS_SOUND(snd)->Stop();
}

/* ---- Element (base) ---- */

extern "C" void solid_bridge_element_set_position(SolidGuiElement *el, int x, int y) {
    if (el) AS_ELEMENT(el)->SetPosition(x, y);
}

extern "C" void solid_bridge_element_set_alpha(SolidGuiElement *el, int alpha) {
    if (el) AS_ELEMENT(el)->SetAlpha(alpha);
}

extern "C" void solid_bridge_element_set_visible(SolidGuiElement *el, bool visible) {
    if (el) AS_ELEMENT(el)->SetVisible(visible);
}

extern "C" void solid_bridge_element_set_scale(SolidGuiElement *el, float scale) {
    if (el) AS_ELEMENT(el)->SetScale(scale);
}

extern "C" void solid_bridge_element_set_size_w(SolidGuiElement *el, int width) {
    if (!el) return;
    GuiElement *e = AS_ELEMENT(el);
    e->SetSize(width, e->GetHeight());
}

extern "C" void solid_bridge_element_set_size_h(SolidGuiElement *el, int height) {
    if (!el) return;
    GuiElement *e = AS_ELEMENT(el);
    e->SetSize(e->GetWidth(), height);
}

extern "C" void solid_bridge_element_set_color(SolidGuiElement *el,
                                                uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    if (!el) return;
    /* Color applies to GuiText — check by attempting dynamic cast isn't
       available without RTTI, so we use SetColor if the element is a text.
       The bridge user (code emitter) knows the concrete type. */
    GuiText *txt = reinterpret_cast<GuiText *>(el);
    GXColor c = {r, g, b, a};
    txt->SetColor(c);
}

extern "C" void solid_bridge_element_set_background_color(SolidGuiElement *el,
                                                           uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    /* Background color creates a colored rectangle behind the element.
       Implemented by wrapping in a GuiImage with the given color.
       For now, this is a no-op placeholder — the emitter should create
       a GuiImage background separately and Append it before the element. */
    (void)el; (void)r; (void)g; (void)b; (void)a;
}

/* ---- Casting helpers ---- */

extern "C" SolidGuiElement *solid_bridge_text_as_element(SolidGuiText *txt) {
    return reinterpret_cast<SolidGuiElement *>(static_cast<GuiElement *>(AS_TEXT(txt)));
}

extern "C" SolidGuiElement *solid_bridge_image_as_element(SolidGuiImage *img) {
    return reinterpret_cast<SolidGuiElement *>(static_cast<GuiElement *>(AS_IMAGE(img)));
}

extern "C" SolidGuiElement *solid_bridge_button_as_element(SolidGuiButton *btn) {
    return reinterpret_cast<SolidGuiElement *>(static_cast<GuiElement *>(AS_BUTTON(btn)));
}

extern "C" SolidGuiElement *solid_bridge_window_as_element(SolidGuiWindow *win) {
    return reinterpret_cast<SolidGuiElement *>(static_cast<GuiElement *>(AS_WINDOW(win)));
}

#else /* !SOLID_HAS_LIBWIIGUI — Host stubs for testing */

#include <cstdio>

extern "C" void solid_bridge_video_init(void) { }
extern "C" void solid_bridge_draw_start(void) { }
extern "C" void solid_bridge_draw_end(void) { }
extern "C" void solid_bridge_input_init(void) { }
extern "C" void solid_bridge_input_scan(void) { }

extern "C" SolidGuiWindow *solid_bridge_window_new(int w, int h) {
    (void)w; (void)h;
    return (SolidGuiWindow *)calloc(1, 64);
}
extern "C" void solid_bridge_window_append(SolidGuiWindow *w, SolidGuiElement *c) { (void)w; (void)c; }
extern "C" void solid_bridge_window_remove(SolidGuiWindow *w, SolidGuiElement *c) { (void)w; (void)c; }
extern "C" void solid_bridge_window_update(SolidGuiWindow *w) { (void)w; }
extern "C" void solid_bridge_window_draw(SolidGuiWindow *w) { (void)w; }

extern "C" SolidGuiText *solid_bridge_text_new(const char *t, int s,
    uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    (void)t; (void)s; (void)r; (void)g; (void)b; (void)a;
    return (SolidGuiText *)calloc(1, 64);
}
extern "C" void solid_bridge_text_set_text(SolidGuiText *t, const char *s) { (void)t; (void)s; }
extern "C" void solid_bridge_text_set_font_size(SolidGuiText *t, int s) { (void)t; (void)s; }
extern "C" void solid_bridge_text_set_max_width(SolidGuiText *t, int w) { (void)t; (void)w; }

extern "C" SolidGuiImageData *solid_bridge_imagedata_new(const uint8_t *d, size_t s) {
    (void)d; (void)s;
    return (SolidGuiImageData *)calloc(1, 64);
}
extern "C" SolidGuiImage *solid_bridge_image_new(SolidGuiImageData *d) {
    (void)d;
    return (SolidGuiImage *)calloc(1, 64);
}
extern "C" SolidGuiImage *solid_bridge_image_new_color(int w, int h,
    uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    (void)w; (void)h; (void)r; (void)g; (void)b; (void)a;
    return (SolidGuiImage *)calloc(1, 64);
}

extern "C" SolidGuiButton *solid_bridge_button_new(int w, int h) {
    (void)w; (void)h;
    return (SolidGuiButton *)calloc(1, 64);
}
extern "C" void solid_bridge_button_set_image(SolidGuiButton *b, SolidGuiImage *i) { (void)b; (void)i; }
extern "C" void solid_bridge_button_set_label(SolidGuiButton *b, SolidGuiText *t) { (void)b; (void)t; }
extern "C" void solid_bridge_button_set_trigger(SolidGuiButton *b) { (void)b; }
extern "C" void solid_bridge_button_set_click_handler(SolidGuiButton *b,
    SolidClickHandler h, void *c) { (void)b; (void)h; (void)c; }
extern "C" bool solid_bridge_button_was_clicked(SolidGuiButton *b) { (void)b; return false; }

extern "C" SolidGuiSound *solid_bridge_sound_new(const uint8_t *d, size_t s) {
    (void)d; (void)s;
    return (SolidGuiSound *)calloc(1, 64);
}
extern "C" void solid_bridge_sound_play(SolidGuiSound *s) { (void)s; }
extern "C" void solid_bridge_sound_stop(SolidGuiSound *s) { (void)s; }

extern "C" void solid_bridge_element_set_position(SolidGuiElement *e, int x, int y) { (void)e; (void)x; (void)y; }
extern "C" void solid_bridge_element_set_alpha(SolidGuiElement *e, int a) { (void)e; (void)a; }
extern "C" void solid_bridge_element_set_visible(SolidGuiElement *e, bool v) { (void)e; (void)v; }
extern "C" void solid_bridge_element_set_scale(SolidGuiElement *e, float s) { (void)e; (void)s; }
extern "C" void solid_bridge_element_set_size_w(SolidGuiElement *e, int w) { (void)e; (void)w; }
extern "C" void solid_bridge_element_set_size_h(SolidGuiElement *e, int h) { (void)e; (void)h; }
extern "C" void solid_bridge_element_set_color(SolidGuiElement *e,
    uint8_t r, uint8_t g, uint8_t b, uint8_t a) { (void)e; (void)r; (void)g; (void)b; (void)a; }
extern "C" void solid_bridge_element_set_background_color(SolidGuiElement *e,
    uint8_t r, uint8_t g, uint8_t b, uint8_t a) { (void)e; (void)r; (void)g; (void)b; (void)a; }

extern "C" SolidGuiElement *solid_bridge_text_as_element(SolidGuiText *t) { return (SolidGuiElement *)t; }
extern "C" SolidGuiElement *solid_bridge_image_as_element(SolidGuiImage *i) { return (SolidGuiElement *)i; }
extern "C" SolidGuiElement *solid_bridge_button_as_element(SolidGuiButton *b) { return (SolidGuiElement *)b; }
extern "C" SolidGuiElement *solid_bridge_window_as_element(SolidGuiWindow *w) { return (SolidGuiElement *)w; }

#endif /* SOLID_HAS_LIBWIIGUI */
