/*
 * solid_control_flow.c — Reactive <Show> and <For> implementation.
 */

#include "solid_control_flow.h"
#include "solid_signal.h"
#include "solid_effect.h"
#include "solid_gui_bridge.h"
#include "calynda_gc.h"
#include "calynda_runtime.h"

#include <string.h>

/* ---- <Show> ---- */

static void show_effect_fn(void *ctx)
{
    SolidShow *show = (SolidShow *)ctx;
    if (!show) return;

    CalyndaRtWord when = solid_signal_get(show->when_signal);
    bool should_show = (when != (CalyndaRtWord)0);

    if (should_show && !show->current_child) {
        /* Render and append */
        show->current_child = show->render_fn(show->render_context);
        if (show->current_child) {
            solid_bridge_window_append(show->parent, show->current_child);
        }
    } else if (!should_show && show->current_child) {
        /* Remove */
        solid_bridge_window_remove(show->parent, show->current_child);
        show->current_child = NULL;
    }
}

SolidShow *solid_show_create(SolidGuiWindow   *parent,
                             SolidSignal      *when_signal,
                             SolidShowRenderFn render_fn,
                             void             *render_context)
{
    SolidShow *show = (SolidShow *)calynda_gc_alloc(sizeof(SolidShow));
    if (!show) return NULL;

    show->parent = parent;
    show->when_signal = when_signal;
    show->render_fn = render_fn;
    show->render_context = render_context;
    show->current_child = NULL;

    /* Create tracking effect — runs immediately to establish initial state */
    show->tracking = solid_create_effect(show_effect_fn, show);

    return show;
}

void solid_show_dispose(SolidShow *show)
{
    if (!show) return;
    if (show->tracking) solid_effect_dispose(show->tracking);
    if (show->current_child && show->parent) {
        solid_bridge_window_remove(show->parent, show->current_child);
    }
    calynda_gc_release(show);
}

/* ---- <For> ---- */

static void for_remove_all(SolidFor *node)
{
    if (!node) return;
    for (size_t i = 0; i < node->child_count; i++) {
        if (node->current_children[i]) {
            solid_bridge_window_remove(node->parent, node->current_children[i]);
        }
    }
    node->child_count = 0;
}

static void for_effect_fn(void *ctx)
{
    SolidFor *node = (SolidFor *)ctx;
    if (!node) return;

    CalyndaRtWord list_word = solid_signal_get(node->list_signal);

    /* Remove old children */
    for_remove_all(node);

    /* Get array length from the runtime */
    size_t count = calynda_rt_array_length(list_word);
    if (count == 0) return;

    /* Grow children array if needed */
    if (count > node->child_capacity) {
        if (node->current_children) calynda_gc_release(node->current_children);
        node->current_children = (SolidGuiElement **)calynda_gc_alloc(
            sizeof(SolidGuiElement *) * count);
        if (!node->current_children) {
            node->child_capacity = 0;
            return;
        }
        node->child_capacity = count;
    }

    /* Render each item */
    for (size_t i = 0; i < count; i++) {
        CalyndaRtWord item = __calynda_rt_index_load(list_word, (CalyndaRtWord)i);
        SolidGuiElement *child = node->render_fn(item, i, node->render_context);
        node->current_children[i] = child;
        if (child) {
            solid_bridge_window_append(node->parent, child);
        }
    }
    node->child_count = count;
}

SolidFor *solid_for_create(SolidGuiWindow  *parent,
                           SolidSignal     *list_signal,
                           SolidForRenderFn render_fn,
                           void            *render_context)
{
    SolidFor *node = (SolidFor *)calynda_gc_alloc(sizeof(SolidFor));
    if (!node) return NULL;

    node->parent = parent;
    node->list_signal = list_signal;
    node->render_fn = render_fn;
    node->render_context = render_context;
    node->current_children = NULL;
    node->child_count = 0;
    node->child_capacity = 0;

    /* Create tracking effect */
    node->tracking = solid_create_effect(for_effect_fn, node);

    return node;
}

void solid_for_dispose(SolidFor *node)
{
    if (!node) return;
    if (node->tracking) solid_effect_dispose(node->tracking);
    for_remove_all(node);
    if (node->current_children) calynda_gc_release(node->current_children);
    calynda_gc_release(node);
}
