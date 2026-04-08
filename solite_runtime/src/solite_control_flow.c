/*
 * solid_control_flow.c — Reactive <Show> and <For> implementation.
 */

#include "solite_control_flow.h"
#include "solite_signal.h"
#include "solite_effect.h"
#include "solite_gui_bridge.h"
#include "calynda_gc.h"
#include "calynda_runtime.h"

#include <string.h>

/* ---- <Show> ---- */

static void show_effect_fn(void *ctx)
{
    SoliteShow *show = (SoliteShow *)ctx;
    if (!show) return;

    CalyndaRtWord when = solite_signal_get(show->when_signal);
    bool should_show = (when != (CalyndaRtWord)0);

    if (should_show && !show->current_child) {
        /* Render and append */
        show->current_child = show->render_fn(show->render_context);
        if (show->current_child) {
            solite_bridge_window_append(show->parent, show->current_child);
        }
    } else if (!should_show && show->current_child) {
        /* Remove */
        solite_bridge_window_remove(show->parent, show->current_child);
        show->current_child = NULL;
    }
}

SoliteShow *solite_show_create(SoliteGuiWindow   *parent,
                             SoliteSignal      *when_signal,
                             SoliteShowRenderFn render_fn,
                             void             *render_context)
{
    SoliteShow *show = (SoliteShow *)calynda_gc_alloc(sizeof(SoliteShow));
    if (!show) return NULL;

    show->parent = parent;
    show->when_signal = when_signal;
    show->render_fn = render_fn;
    show->render_context = render_context;
    show->current_child = NULL;

    /* Create tracking effect — runs immediately to establish initial state */
    show->tracking = solite_create_effect(show_effect_fn, show);

    return show;
}

void solite_show_dispose(SoliteShow *show)
{
    if (!show) return;
    if (show->tracking) solite_effect_dispose(show->tracking);
    if (show->current_child && show->parent) {
        solite_bridge_window_remove(show->parent, show->current_child);
    }
    calynda_gc_release(show);
}

/* ---- <For> ---- */

static void for_remove_all(SoliteFor *node)
{
    if (!node) return;
    for (size_t i = 0; i < node->child_count; i++) {
        if (node->current_children[i]) {
            solite_bridge_window_remove(node->parent, node->current_children[i]);
        }
    }
    node->child_count = 0;
}

static void for_effect_fn(void *ctx)
{
    SoliteFor *node = (SoliteFor *)ctx;
    if (!node) return;

    CalyndaRtWord list_word = solite_signal_get(node->list_signal);

    /* Remove old children */
    for_remove_all(node);

    /* Get array length from the runtime */
    size_t count = calynda_rt_array_length(list_word);
    if (count == 0) return;

    /* Grow children array if needed */
    if (count > node->child_capacity) {
        if (node->current_children) calynda_gc_release(node->current_children);
        node->current_children = (SoliteGuiElement **)calynda_gc_alloc(
            sizeof(SoliteGuiElement *) * count);
        if (!node->current_children) {
            node->child_capacity = 0;
            return;
        }
        node->child_capacity = count;
    }

    /* Render each item */
    for (size_t i = 0; i < count; i++) {
        CalyndaRtWord item = __calynda_rt_index_load(list_word, (CalyndaRtWord)i);
        SoliteGuiElement *child = node->render_fn(item, i, node->render_context);
        node->current_children[i] = child;
        if (child) {
            solite_bridge_window_append(node->parent, child);
        }
    }
    node->child_count = count;
}

SoliteFor *solite_for_create(SoliteGuiWindow  *parent,
                           SoliteSignal     *list_signal,
                           SoliteForRenderFn render_fn,
                           void            *render_context)
{
    SoliteFor *node = (SoliteFor *)calynda_gc_alloc(sizeof(SoliteFor));
    if (!node) return NULL;

    node->parent = parent;
    node->list_signal = list_signal;
    node->render_fn = render_fn;
    node->render_context = render_context;
    node->current_children = NULL;
    node->child_count = 0;
    node->child_capacity = 0;

    /* Create tracking effect */
    node->tracking = solite_create_effect(for_effect_fn, node);

    return node;
}

void solite_for_dispose(SoliteFor *node)
{
    if (!node) return;
    if (node->tracking) solite_effect_dispose(node->tracking);
    for_remove_all(node);
    if (node->current_children) calynda_gc_release(node->current_children);
    calynda_gc_release(node);
}
