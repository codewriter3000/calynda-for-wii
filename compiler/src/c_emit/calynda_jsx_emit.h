#ifndef CALYNDA_JSX_EMIT_H
#define CALYNDA_JSX_EMIT_H

/*
 * calynda_jsx_emit.h — C code emitter for JSX elements.
 *
 * Transforms a JsxElement tree into C code that calls the
 * solid_bridge_* and solid_* runtime functions.
 *
 * Example JSX:
 *   <Window width={640} height={480}>
 *     <Text style={{ color: "#FFFFFF" }}>{`Count: ${count()}`}</Text>
 *   </Window>
 *
 * Emits:
 *   SolidGuiWindow *__jsx_0 = solid_bridge_window_new(640, 480);
 *   SolidGuiText *__jsx_1 = solid_bridge_text_new("", 20, 255, 255, 255, 255);
 *   solid_bridge_element_set_color(
 *       solid_bridge_text_as_element(__jsx_1), 255, 255, 255, 255);
 *   solid_create_effect(__jsx_1_text_effect, &__jsx_1_ctx);
 *   solid_bridge_window_append(__jsx_0,
 *       solid_bridge_text_as_element(__jsx_1));
 */

#include "calynda_jsx_ast.h"
#include <stdbool.h>
#include <stdio.h>

typedef struct {
    FILE *out;
    int   indent;
    int   element_counter;   /* unique ID for each JSX element */
    int   effect_counter;    /* unique ID for each reactive effect */
} JsxEmitContext;

/*
 * Initialize the JSX emit context.
 */
void jsx_emit_init(JsxEmitContext *ctx, FILE *out);

/*
 * Emit C code for a complete JSX element tree.
 * Returns the variable name of the root element (e.g. "__jsx_0").
 */
bool jsx_emit_element(JsxEmitContext *ctx, const JsxElement *element,
                      const char *parent_var);

/*
 * Emit a complete component function that returns a GUI element tree.
 */
bool jsx_emit_component(JsxEmitContext *ctx, const char *component_name,
                        const JsxElement *root);

#endif /* CALYNDA_JSX_EMIT_H */
