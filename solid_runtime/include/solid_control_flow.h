#ifndef SOLID_CONTROL_FLOW_H
#define SOLID_CONTROL_FLOW_H

/*
 * solid_control_flow.h — Reactive control flow primitives for Solid/Wii.
 *
 * Implements <Show> and <For> as C-level constructs:
 *   - SolidShow: conditionally adds/removes a child element
 *   - SolidFor:  maps a signal-backed list to child elements
 *
 * Both integrate with the reactive system and the libwiigui bridge.
 */

#include "solid_signal.h"
#include "solid_effect.h"
#include "solid_gui_bridge.h"
#include "calynda_runtime.h"

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---- <Show when={signal}> ---- */

/*
 * Render callback: creates a GUI element to display.
 * Returns an opaque SolidGuiElement* (cast from GuiElement* by bridge).
 */
typedef SolidGuiElement *(*SolidShowRenderFn)(void *context);

typedef struct SolidShow {
    SolidGuiWindow   *parent;
    SolidSignal      *when_signal;
    SolidShowRenderFn render_fn;
    void             *render_context;
    SolidGuiElement  *current_child; /* NULL if hidden */
    SolidEffect      *tracking;
} SolidShow;

/*
 * Create a <Show> control flow node.
 * When when_signal is truthy, render_fn is called and the result
 * is appended to parent. When falsy, the child is removed.
 */
SolidShow *solid_show_create(SolidGuiWindow   *parent,
                             SolidSignal      *when_signal,
                             SolidShowRenderFn render_fn,
                             void             *render_context);

void solid_show_dispose(SolidShow *show);

/* ---- <For each={signal}> ---- */

/*
 * Render callback for each item in the list.
 * item: the CalyndaRtWord value of the current list element.
 * index: the index in the list.
 */
typedef SolidGuiElement *(*SolidForRenderFn)(CalyndaRtWord item,
                                             size_t index,
                                             void *context);

typedef struct SolidFor {
    SolidGuiWindow   *parent;
    SolidSignal      *list_signal; /* value is a CalyndaRtArray */
    SolidForRenderFn  render_fn;
    void             *render_context;
    SolidGuiElement **current_children;
    size_t            child_count;
    size_t            child_capacity;
    SolidEffect      *tracking;
} SolidFor;

/*
 * Create a <For> control flow node.
 * When list_signal changes, the old children are removed and new ones
 * are rendered from the updated list.
 */
SolidFor *solid_for_create(SolidGuiWindow  *parent,
                           SolidSignal     *list_signal,
                           SolidForRenderFn render_fn,
                           void            *render_context);

void solid_for_dispose(SolidFor *node);

#ifdef __cplusplus
}
#endif

#endif /* SOLID_CONTROL_FLOW_H */
