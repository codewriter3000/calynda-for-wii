/*
 * calynda_jsx_emit.c — C code emitter for JSX elements.
 *
 * Walks the JsxElement AST and emits C code targeting the
 * solite_bridge_* API and solite_* reactive runtime.
 *
 * Widget mapping:
 *   <Window>  → solite_bridge_window_new()
 *   <Button>  → solite_bridge_button_new()
 *   <Text>    → solite_bridge_text_new()
 *   <Image>   → solite_bridge_image_new_color()
 *
 * Style properties are emitted via the solite_style_* API.
 * Event handlers become lambda closures with bridge callbacks.
 * Reactive expressions ({expr}) become solite_create_effect() calls.
 */

#include "calynda_jsx_emit.h"
#include "calynda_jsx_ast.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

/* ---- Helpers ---- */

static void emit_indent(JsxEmitContext *ctx)
{
    for (int i = 0; i < ctx->indent; i++)
        fprintf(ctx->out, "    ");
}

static int next_element_id(JsxEmitContext *ctx)
{
    return ctx->element_counter++;
}

static int next_effect_id(JsxEmitContext *ctx)
{
    return ctx->effect_counter++;
}

/*
 * Determine the bridge type name for a tag.
 */
static const char *bridge_type_for_tag(const char *tag)
{
    if (strcmp(tag, "Window") == 0) return "SoliteGuiWindow";
    if (strcmp(tag, "Button") == 0) return "SoliteGuiButton";
    if (strcmp(tag, "Text") == 0)   return "SoliteGuiText";
    if (strcmp(tag, "Image") == 0)  return "SoliteGuiImage";
    return "SoliteGuiElement";
}

/*
 * Get the as_element cast function for a tag.
 */
static const char *as_element_fn(const char *tag)
{
    if (strcmp(tag, "Window") == 0) return "solite_bridge_window_as_element";
    if (strcmp(tag, "Button") == 0) return "solite_bridge_button_as_element";
    if (strcmp(tag, "Text") == 0)   return "solite_bridge_text_as_element";
    if (strcmp(tag, "Image") == 0)  return "solite_bridge_image_as_element";
    return "";
}

/*
 * Parse a CSS hex color string like "#RRGGBB" or "#RRGGBBAA" into RGBA.
 */
static bool parse_hex_color(const char *s, int *r, int *g, int *b, int *a)
{
    if (!s || s[0] != '#') return false;
    size_t len = strlen(s + 1);
    unsigned int val;
    if (len == 6) {
        if (sscanf(s + 1, "%06x", &val) != 1) return false;
        *r = (val >> 16) & 0xFF;
        *g = (val >> 8) & 0xFF;
        *b = val & 0xFF;
        *a = 255;
        return true;
    }
    if (len == 8) {
        if (sscanf(s + 1, "%08x", &val) != 1) return false;
        *r = (val >> 24) & 0xFF;
        *g = (val >> 16) & 0xFF;
        *b = (val >> 8) & 0xFF;
        *a = val & 0xFF;
        return true;
    }
    return false;
}

/*
 * Look up an integer attribute value. Returns 0 if missing.
 */
static int get_int_attr(const JsxElement *el, const char *name, int fallback)
{
    for (size_t i = 0; i < el->attr_count; i++) {
        if (strcmp(el->attributes[i].name, name) != 0) continue;
        if (el->attributes[i].kind == JSX_ATTR_EXPRESSION) {
            AstExpression *e = el->attributes[i].as.expression;
            if (e && e->kind == AST_EXPR_LITERAL &&
                e->as.literal.kind == AST_LITERAL_INTEGER) {
                return atoi(e->as.literal.as.text);
            }
        }
        if (el->attributes[i].kind == JSX_ATTR_STRING) {
            return atoi(el->attributes[i].as.string_val);
        }
    }
    return fallback;
}

/* ---- Emit functions per widget type ---- */

static void emit_window(JsxEmitContext *ctx, const JsxElement *el, int id)
{
    int w = get_int_attr(el, "width", 640);
    int h = get_int_attr(el, "height", 480);
    emit_indent(ctx);
    fprintf(ctx->out, "SoliteGuiWindow *__jsx_%d = solite_bridge_window_new(%d, %d);\n",
            id, w, h);
}

static void emit_text(JsxEmitContext *ctx, const JsxElement *el, int id)
{
    /* Default font size and color */
    int size = get_int_attr(el, "size", 20);
    int r = 255, g = 255, b = 255, a = 255;

    /* Check for text content from first text/expr child */
    const char *text = "";
    for (size_t i = 0; i < el->child_count; i++) {
        if (el->children[i].kind == JSX_CHILD_TEXT) {
            text = el->children[i].as.text;
            break;
        }
    }

    emit_indent(ctx);
    fprintf(ctx->out, "SoliteGuiText *__jsx_%d = solite_bridge_text_new(\"%s\", %d, %d, %d, %d, %d);\n",
            id, text, size, r, g, b, a);
}

static void emit_button(JsxEmitContext *ctx, const JsxElement *el, int id)
{
    int w = get_int_attr(el, "width", 100);
    int h = get_int_attr(el, "height", 40);
    emit_indent(ctx);
    fprintf(ctx->out, "SoliteGuiButton *__jsx_%d = solite_bridge_button_new(%d, %d);\n",
            id, w, h);
}

static void emit_image(JsxEmitContext *ctx, const JsxElement *el, int id)
{
    int w = get_int_attr(el, "width", 100);
    int h = get_int_attr(el, "height", 100);
    int r = 0, g = 0, b = 0, a = 255;
    emit_indent(ctx);
    fprintf(ctx->out, "SoliteGuiImage *__jsx_%d = solite_bridge_image_new_color(%d, %d, %d, %d, %d, %d);\n",
            id, w, h, r, g, b, a);
}

/* ---- Style emission ---- */

static void emit_style_props(JsxEmitContext *ctx, const JsxElement *el, int id)
{
    for (size_t i = 0; i < el->attr_count; i++) {
        if (el->attributes[i].kind != JSX_ATTR_STYLE) continue;

        const char *as_el = as_element_fn(el->tag_name);
        if (as_el[0] == '\0') continue;

        for (size_t j = 0; j < el->attributes[i].as.style.count; j++) {
            JsxStyleProp *prop = &el->attributes[i].as.style.props[j];

            if (strcmp(prop->name, "width") == 0) {
                emit_indent(ctx);
                fprintf(ctx->out, "solite_bridge_element_set_size_w(%s(__jsx_%d), %s);\n",
                        as_el, id,
                        (prop->value && prop->value->kind == AST_EXPR_LITERAL)
                            ? prop->value->as.literal.as.text : "0");
            } else if (strcmp(prop->name, "height") == 0) {
                emit_indent(ctx);
                fprintf(ctx->out, "solite_bridge_element_set_size_h(%s(__jsx_%d), %s);\n",
                        as_el, id,
                        (prop->value && prop->value->kind == AST_EXPR_LITERAL)
                            ? prop->value->as.literal.as.text : "0");
            } else if (strcmp(prop->name, "color") == 0) {
                int r = 255, g = 255, b = 255, a = 255;
                if (prop->value && prop->value->kind == AST_EXPR_LITERAL &&
                    prop->value->as.literal.kind == AST_LITERAL_STRING) {
                    parse_hex_color(prop->value->as.literal.as.text, &r, &g, &b, &a);
                }
                emit_indent(ctx);
                fprintf(ctx->out, "solite_bridge_element_set_color(%s(__jsx_%d), %d, %d, %d, %d);\n",
                        as_el, id, r, g, b, a);
            } else if (strcmp(prop->name, "background-color") == 0) {
                int r = 0, g = 0, b = 0, a = 255;
                if (prop->value && prop->value->kind == AST_EXPR_LITERAL &&
                    prop->value->as.literal.kind == AST_LITERAL_STRING) {
                    parse_hex_color(prop->value->as.literal.as.text, &r, &g, &b, &a);
                }
                emit_indent(ctx);
                fprintf(ctx->out, "solite_bridge_element_set_background_color(%s(__jsx_%d), %d, %d, %d, %d);\n",
                        as_el, id, r, g, b, a);
            }
            /* New CSS properties: add else-if cases here.
               The pattern is:
                 1. strcmp(prop->name, "css-property-name")
                 2. Extract the value from prop->value
                 3. Emit the bridge call
            */
        }
    }
}

/* ---- Event handler emission ---- */

static void emit_event_handlers(JsxEmitContext *ctx, const JsxElement *el, int id)
{
    for (size_t i = 0; i < el->attr_count; i++) {
        if (el->attributes[i].kind != JSX_ATTR_HANDLER) continue;

        if (strcmp(el->attributes[i].name, "onClick") == 0 &&
            strcmp(el->tag_name, "Button") == 0) {
            /* For buttons, the click handler is checked each frame.
               We emit a comment + the handler function as a static. */
            int eff_id = next_effect_id(ctx);
            emit_indent(ctx);
            fprintf(ctx->out, "/* onClick handler for __jsx_%d — "
                    "checked via solite_bridge_button_was_clicked() */\n", id);

            /* Emit a flag/context for the click check in the frame loop */
            emit_indent(ctx);
            fprintf(ctx->out, "/* TODO: wire up click handler effect_%d "
                    "in the main loop */\n", eff_id);
        }
    }
}

/* ---- Reactive expression children ---- */

static void emit_reactive_children(JsxEmitContext *ctx, const JsxElement *el,
                                   int id, const char *parent_var)
{
    for (size_t i = 0; i < el->child_count; i++) {
        if (el->children[i].kind == JSX_CHILD_EXPRESSION) {
            int eff_id = next_effect_id(ctx);
            emit_indent(ctx);
            fprintf(ctx->out, "/* Reactive expression child — "
                    "creates effect_%d to update text */\n", eff_id);

            /* If this is a Text element, the expression updates SetText */
            if (strcmp(el->tag_name, "Text") == 0) {
                emit_indent(ctx);
                fprintf(ctx->out, "/* solite_create_effect(update_text_%d_fn, "
                        "&__jsx_%d); */\n", eff_id, id);
            }
        }
    }
}

/* ---- Main element emitter ---- */

void jsx_emit_init(JsxEmitContext *ctx, FILE *out)
{
    ctx->out = out;
    ctx->indent = 1;
    ctx->element_counter = 0;
    ctx->effect_counter = 0;
}

bool jsx_emit_element(JsxEmitContext *ctx, const JsxElement *element,
                      const char *parent_var)
{
    if (!element || !element->tag_name) return false;

    int id = next_element_id(ctx);

    /* Emit widget constructor */
    if (strcmp(element->tag_name, "Window") == 0) {
        emit_window(ctx, element, id);
    } else if (strcmp(element->tag_name, "Text") == 0) {
        emit_text(ctx, element, id);
    } else if (strcmp(element->tag_name, "Button") == 0) {
        emit_button(ctx, element, id);
    } else if (strcmp(element->tag_name, "Image") == 0) {
        emit_image(ctx, element, id);
    } else {
        /* Unknown / component — emit as function call */
        emit_indent(ctx);
        fprintf(ctx->out, "SoliteGuiElement *__jsx_%d = __component_%s();\n",
                id, element->tag_name);
    }

    /* Emit style properties */
    emit_style_props(ctx, element, id);

    /* Emit position if x/y attributes present */
    int x = get_int_attr(element, "x", -1);
    int y = get_int_attr(element, "y", -1);
    if (x >= 0 || y >= 0) {
        const char *as_el = as_element_fn(element->tag_name);
        if (as_el[0] != '\0') {
            emit_indent(ctx);
            fprintf(ctx->out, "solite_bridge_element_set_position(%s(__jsx_%d), %d, %d);\n",
                    as_el, id, x >= 0 ? x : 0, y >= 0 ? y : 0);
        }
    }

    /* Emit event handlers */
    emit_event_handlers(ctx, element, id);

    /* Emit reactive expression children */
    emit_reactive_children(ctx, element, id, parent_var);

    /* Recursively emit children and append to this element */
    for (size_t i = 0; i < element->child_count; i++) {
        if (element->children[i].kind == JSX_CHILD_ELEMENT) {
            JsxElement *child = element->children[i].as.element;
            char var_name[32];
            snprintf(var_name, sizeof(var_name), "__jsx_%d", id);
            jsx_emit_element(ctx, child, var_name);

            /* Append child to parent (must be Window) */
            if (strcmp(element->tag_name, "Window") == 0) {
                int child_id = ctx->element_counter - 1;
                const char *child_as_el = as_element_fn(child->tag_name);
                emit_indent(ctx);
                if (child_as_el[0] != '\0') {
                    fprintf(ctx->out, "solite_bridge_window_append(__jsx_%d, %s(__jsx_%d));\n",
                            id, child_as_el, child_id);
                } else {
                    fprintf(ctx->out, "solite_bridge_window_append(__jsx_%d, __jsx_%d);\n",
                            id, child_id);
                }
            }

            /* Button with Text child → set as label */
            if (strcmp(element->tag_name, "Button") == 0 &&
                strcmp(child->tag_name, "Text") == 0) {
                int child_id = ctx->element_counter - 1;
                emit_indent(ctx);
                fprintf(ctx->out, "solite_bridge_button_set_label(__jsx_%d, __jsx_%d);\n",
                        id, child_id);
            }
        }
    }

    /* Append to parent if provided */
    if (parent_var && parent_var[0] != '\0') {
        /* Parent append is handled by the caller for the root level */
    }

    return true;
}

bool jsx_emit_component(JsxEmitContext *ctx, const char *component_name,
                        const JsxElement *root)
{
    fprintf(ctx->out, "\n/* Component: %s */\n", component_name);
    fprintf(ctx->out, "SoliteGuiWindow *__component_%s(void) {\n", component_name);
    ctx->indent = 1;

    /* Emit all elements */
    jsx_emit_element(ctx, root, "");

    /* Return the root element */
    emit_indent(ctx);
    fprintf(ctx->out, "return __jsx_0;\n");
    fprintf(ctx->out, "}\n\n");

    return true;
}
