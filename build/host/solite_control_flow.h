#ifndef SOLITE_CONTROL_FLOW_H
#define SOLITE_CONTROL_FLOW_H

/*
 * solite_control_flow.h — Reactive control flow primitives for Solite/Wii.
 *
 * Implements <Show> and <For> as C-level constructs:
 *   - SoliteShow: conditionally adds/removes a child element
 *   - SoliteFor:  maps a signal-backed list to child elements
 *
 * Both integrate with the reactive system and the libwiigui bridge.
 */

#include "solite_signal.h"
#include "solite_effect.h"
#include "solite_gui_bridge.h"
#include "calynda_runtime.h"

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---- <Show when={signal}> ---- */

/*
 * Render callback: creates a GUI element to display.
 * Returns an opaque SoliteGuiElement* (cast from GuiElement* by bridge).
 */
typedef SoliteGuiElement *(*SoliteShowRenderFn)(void *context);

typedef struct SoliteShow {
    SoliteGuiWindow   *parent;
    SoliteSignal      *when_signal;
    SoliteShowRenderFn render_fn;
    void             *render_context;
    SoliteGuiElement  *current_child; /* NULL if hidden */
    SoliteEffect      *tracking;
} SoliteShow;

/*
 * Create a <Show> control flow node.
 * When when_signal is truthy, render_fn is called and the result
 * is appended to parent. When falsy, the child is removed.
 */
SoliteShow *solite_show_create(SoliteGuiWindow   *parent,
                             SoliteSignal      *when_signal,
                             SoliteShowRenderFn render_fn,
                             void             *render_context);

void solite_show_dispose(SoliteShow *show);

/* ---- <For each={signal}> ---- */

/*
 * Render callback for each item in the list.
 * item: the CalyndaRtWord value of the current list element.
 * index: the index in the list.
 */
typedef SoliteGuiElement *(*SoliteForRenderFn)(CalyndaRtWord item,
                                             size_t index,
                                             void *context);

typedef struct SoliteFor {
    SoliteGuiWindow   *parent;
    SoliteSignal      *list_signal; /* value is a CalyndaRtArray */
    SoliteForRenderFn  render_fn;
    void             *render_context;
    SoliteGuiElement **current_children;
    size_t            child_count;
    size_t            child_capacity;
    SoliteEffect      *tracking;
} SoliteFor;

/*
 * Create a <For> control flow node.
 * When list_signal changes, the old children are removed and new ones
 * are rendered from the updated list.
 */
SoliteFor *solite_for_create(SoliteGuiWindow  *parent,
                           SoliteSignal     *list_signal,
                           SoliteForRenderFn render_fn,
                           void            *render_context);

void solite_for_dispose(SoliteFor *node);

#ifdef __cplusplus
}
#endif

#endif /* SOLITE_CONTROL_FLOW_H */
