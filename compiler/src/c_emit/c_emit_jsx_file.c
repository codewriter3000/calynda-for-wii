/*
 * c_emit_jsx_file.c — Full-file Calynda+JSX to C transpiler.
 *
 * This module provides an alternative compilation path for Calynda
 * source files that contain Solid-inspired component/signal/JSX syntax.
 * It bypasses the normal parser→sema→HIR→emit pipeline and instead:
 *
 *   1. Tokenizes the entire file with the JSX-aware tokenizer.
 *   2. Walks the token stream to parse component + boot declarations.
 *   3. Emits C code targeting the solid_bridge_* / solid_* APIs.
 *
 * Supported syntax:
 *   import io.gui;
 *   component Counter = () -> {
 *       signal count = 0;
 *       return (<Window>...</Window>);
 *   };
 *   boot() -> { gui.mount(Counter()); };
 */

#define _GNU_SOURCE

#include "c_emit_jsx_file.h"
#include "calynda_jsx_tokens.h"
#include "calynda_jsx_parser.h"
#include "calynda_jsx_ast.h"
#include "calynda_jsx_emit.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ================================================================== */
/*  Internal types                                                    */
/* ================================================================== */

#define MAX_SIGNALS    32
#define MAX_COMPONENTS 16
#define MAX_HANDLERS   64

typedef struct {
    char *name;           /* signal identifier  */
    char *init_text;      /* initializer source */
} SignalInfo;

typedef struct {
    char       *comp_name;             /* component name           */
    SignalInfo  signals[MAX_SIGNALS];
    int         signal_count;
    JsxElement *root_element;          /* parsed JSX tree          */
} ComponentInfo;

typedef struct {
    char *body_text;      /* raw text of handler expression */
    int   handler_id;
} HandlerInfo;

typedef struct {
    FILE          *out;
    ComponentInfo  components[MAX_COMPONENTS];
    int            component_count;
    HandlerInfo    handlers[MAX_HANDLERS];
    int            handler_count;
    int            handler_id_counter;
} JsxFileContext;

/* ================================================================== */
/*  Helpers                                                           */
/* ================================================================== */

static char *jsx_dup_str(const char *s, size_t len)
{
    char *d = (char *)malloc(len + 1);
    if (d) { memcpy(d, s, len); d[len] = '\0'; }
    return d;
}

static bool jsx_expect(JsxTokenizer *jt, int expected_type)
{
    JsxToken t = jsx_tokenizer_next(jt);
    return t.jsx_type == expected_type;
}

static void jsx_skip_to_semicolon(JsxTokenizer *jt)
{
    JsxToken t;
    do {
        t = jsx_tokenizer_next(jt);
    } while (t.jsx_type != TOK_SEMICOLON && t.jsx_type != TOK_EOF);
}

/* ================================================================== */
/*  Signal-aware expression rewriting                                 */
/* ================================================================== */

/*
 * Rewrite a raw expression string, replacing signal references:
 *   count()             → (int)solid_signal_get(sig_count)
 *   set count(expr)     → solid_signal_set(sig_count, (CalyndaRtWord)(expr))
 */
static char *rewrite_signal_expr(const char *raw, const SignalInfo *signals,
                                 int signal_count)
{
    if (!raw || signal_count == 0) return raw ? strdup(raw) : NULL;

    /* Work buffer — generously sized */
    size_t cap = strlen(raw) * 4 + 256;
    char *buf = (char *)malloc(cap);
    if (!buf) return strdup(raw);
    size_t pos = 0;
    const char *p = raw;

#define APPEND(s, n) do {           \
    if (pos + (n) >= cap) {         \
        cap *= 2;                   \
        buf = realloc(buf, cap);    \
    }                               \
    memcpy(buf + pos, (s), (n));    \
    pos += (n);                     \
} while(0)

#define APPENDS(s) APPEND((s), strlen(s))

    while (*p) {
        bool matched = false;

        /* Check for "set signalName(" */
        if (strncmp(p, "set ", 4) == 0) {
            const char *after_set = p + 4;
            /* skip spaces */
            while (*after_set == ' ') after_set++;
            for (int i = 0; i < signal_count; i++) {
                size_t nlen = strlen(signals[i].name);
                if (strncmp(after_set, signals[i].name, nlen) == 0 &&
                    after_set[nlen] == '(') {
                    /* Found "set name(" — find matching ')' */
                    const char *arg_start = after_set + nlen + 1;
                    int depth = 1;
                    const char *q = arg_start;
                    while (*q && depth > 0) {
                        if (*q == '(') depth++;
                        else if (*q == ')') depth--;
                        if (depth > 0) q++;
                    }
                    /* arg_start..q is the argument */
                    char *inner = jsx_dup_str(arg_start, (size_t)(q - arg_start));
                    char *rewritten_inner = rewrite_signal_expr(inner, signals,
                                                                signal_count);
                    APPENDS("solid_signal_set(sig_");
                    APPEND(signals[i].name, nlen);
                    APPENDS(", (CalyndaRtWord)(");
                    APPENDS(rewritten_inner);
                    APPENDS("))");
                    free(inner);
                    free(rewritten_inner);
                    p = q + 1; /* skip past ')' */
                    matched = true;
                    break;
                }
            }
        }

        /* Check for "signalName()" — signal getter */
        if (!matched) {
            for (int i = 0; i < signal_count; i++) {
                size_t nlen = strlen(signals[i].name);
                /* Must be at word boundary */
                bool at_boundary = (p == raw) ||
                    (!isalnum((unsigned char)p[-1]) && p[-1] != '_');
                if (at_boundary &&
                    strncmp(p, signals[i].name, nlen) == 0 &&
                    p[nlen] == '(' && p[nlen + 1] == ')') {
                    APPENDS("(int)solid_signal_get(sig_");
                    APPEND(signals[i].name, nlen);
                    APPENDS(")");
                    p += nlen + 2; /* skip "name()" */
                    matched = true;
                    break;
                }
            }
        }

        if (!matched) {
            APPEND(p, 1);
            p++;
        }
    }

    buf[pos] = '\0';
    return buf;

#undef APPEND
#undef APPENDS
}

/* ================================================================== */
/*  Handler emission                                                  */
/* ================================================================== */

/*
 * Walk a JsxElement tree and extract onClick handlers.
 * Emits static handler functions, and records handler IDs
 * so the element emitter can wire them up later.
 */
static void extract_and_emit_handlers(JsxFileContext *fctx,
                                      const JsxElement *el,
                                      const SignalInfo *signals,
                                      int signal_count,
                                      int element_id_base)
{
    /* Recurse into children first to keep IDs in sync with emit order */
    int child_id = element_id_base + 1;
    for (size_t i = 0; i < el->child_count; i++) {
        if (el->children[i].kind == JSX_CHILD_ELEMENT) {
            extract_and_emit_handlers(fctx, el->children[i].as.element,
                                      signals, signal_count, child_id);
            /* Count child subtree size (approximate) */
            child_id++;
        }
    }

    /* Look for onClick handler */
    for (size_t i = 0; i < el->attr_count; i++) {
        if (el->attributes[i].kind == JSX_ATTR_HANDLER &&
            strcmp(el->attributes[i].name, "onClick") == 0 &&
            el->attributes[i].raw_text) {
            int hid = fctx->handler_id_counter++;
            char *raw = el->attributes[i].raw_text;

            /* The raw text is like "() -> set count(count() + 1)"
               Extract the body after "() ->" */
            const char *body = raw;
            const char *arrow = strstr(raw, "->");
            if (arrow) body = arrow + 2;
            while (*body == ' ') body++;

            char *rewritten = rewrite_signal_expr(body, signals, signal_count);

            fprintf(fctx->out,
                    "static void __handler_%d(const char **buttons, int button_count, void *user_data) {\n"
                    "    (void)buttons; (void)button_count; (void)user_data;\n"
                    "    %s;\n"
                    "}\n\n",
                    hid, rewritten);

            free(rewritten);

            /* Record for wiring */
            if (fctx->handler_count < MAX_HANDLERS) {
                fctx->handlers[fctx->handler_count].handler_id = hid;
                fctx->handlers[fctx->handler_count].body_text = raw;
                fctx->handler_count++;
            }
        }
    }
}

/* ================================================================== */
/*  Reactive-expression (template) effect emission                    */
/* ================================================================== */

/*
 * For Text elements with expression children (like {`Count: ${count()}`}),
 * emit a reactive effect function that updates the text when signals change.
 */
static void emit_text_effects(FILE *out, const JsxElement *el,
                              const SignalInfo *signals, int signal_count,
                              int element_id, int *effect_counter)
{
    if (strcmp(el->tag_name, "Text") != 0) return;

    for (size_t i = 0; i < el->child_count; i++) {
        if (el->children[i].kind != JSX_CHILD_EXPRESSION) continue;
        if (!el->children[i].raw_text) continue;

        const char *raw = el->children[i].raw_text;

        /* Check if it's a template literal with signal references */
        bool has_signal_ref = false;
        for (int s = 0; s < signal_count; s++) {
            if (strstr(raw, signals[s].name)) {
                has_signal_ref = true;
                break;
            }
        }
        if (!has_signal_ref) continue;

        int eid = (*effect_counter)++;

        /* Template literal: `Count: ${count()}`
         * The raw_text is the full template including backticks.
         * We need to transform it into a snprintf pattern. */

        /* Parse template: `prefix${expr}suffix` */
        if (raw[0] == '`') {
            /* Build format string and arguments */
            fprintf(out,
                    "typedef struct { SolidGuiText *text; } __eff_%d_ctx_t;\n"
                    "static __eff_%d_ctx_t __eff_%d_ctx;\n",
                    eid, eid, eid);

            fprintf(out,
                    "static void __effect_%d_fn(void *ud) {\n"
                    "    __eff_%d_ctx_t *ctx = (__eff_%d_ctx_t *)ud;\n"
                    "    char buf[128];\n",
                    eid, eid, eid);

            /* Simple template parse: split on ${ and } */
            const char *p = raw + 1; /* skip opening ` */
            char fmt[256] = {0};
            char args[512] = {0};
            size_t fpos = 0;
            size_t apos = 0;

            while (*p && *p != '`') {
                if (p[0] == '$' && p[1] == '{') {
                    p += 2; /* skip ${ */
                    const char *expr_start = p;
                    int depth = 1;
                    while (*p && depth > 0) {
                        if (*p == '{') depth++;
                        else if (*p == '}') depth--;
                        if (depth > 0) p++;
                    }
                    char *expr = jsx_dup_str(expr_start, (size_t)(p - expr_start));
                    char *rewritten = rewrite_signal_expr(expr, signals,
                                                          signal_count);
                    /* Add %d format specifier and argument */
                    fmt[fpos++] = '%';
                    fmt[fpos++] = 'd';
                    if (apos > 0) { args[apos++] = ','; args[apos++] = ' '; }
                    size_t rlen = strlen(rewritten);
                    memcpy(args + apos, rewritten, rlen);
                    apos += rlen;
                    free(expr);
                    free(rewritten);
                    if (*p == '}') p++; /* skip closing } */
                } else {
                    fmt[fpos++] = *p;
                    p++;
                }
            }
            fmt[fpos] = '\0';
            args[apos] = '\0';

            fprintf(out,
                    "    snprintf(buf, sizeof(buf), \"%s\", %s);\n"
                    "    solid_bridge_text_set_text(ctx->text, buf);\n"
                    "}\n\n",
                    fmt, args);
        } else {
            /* Bare signal reference: {count}
             * raw_text is just the identifier, e.g. "count".
             * Append "()" so rewrite_signal_expr recognises the getter. */
            size_t rlen = strlen(raw);
            char *getter = (char *)malloc(rlen + 3);
            memcpy(getter, raw, rlen);
            getter[rlen] = '(';
            getter[rlen + 1] = ')';
            getter[rlen + 2] = '\0';
            char *rewritten = rewrite_signal_expr(getter, signals, signal_count);
            free(getter);

            fprintf(out,
                    "typedef struct { SolidGuiText *text; } __eff_%d_ctx_t;\n"
                    "static __eff_%d_ctx_t __eff_%d_ctx;\n",
                    eid, eid, eid);

            fprintf(out,
                    "static void __effect_%d_fn(void *ud) {\n"
                    "    __eff_%d_ctx_t *ctx = (__eff_%d_ctx_t *)ud;\n"
                    "    char buf[128];\n"
                    "    snprintf(buf, sizeof(buf), \"%%d\", %s);\n"
                    "    solid_bridge_text_set_text(ctx->text, buf);\n"
                    "}\n\n",
                    eid, eid, eid, rewritten);

            free(rewritten);
        }
    }
}

/* ================================================================== */
/*  Enhanced element emitter with signal awareness                    */
/* ================================================================== */

/*
 * Emit C code for a JSX element tree, with full signal/handler support.
 * This replaces jsx_emit_element for the file-level compilation path.
 */
static int emit_jsx_element_full(JsxFileContext *fctx,
                                 const JsxElement *el,
                                 const char *parent_var,
                                 const SignalInfo *signals,
                                 int signal_count,
                                 int *id_counter,
                                 int *effect_counter,
                                 int handler_idx)
{
    FILE *out = fctx->out;
    int id = (*id_counter)++;

    /* Widget constructor */
    if (strcmp(el->tag_name, "Window") == 0) {
        int w = 640, h = 480;
        for (size_t i = 0; i < el->attr_count; i++) {
            if (el->attributes[i].kind == JSX_ATTR_EXPRESSION &&
                el->attributes[i].as.expression &&
                el->attributes[i].as.expression->kind == AST_EXPR_LITERAL &&
                el->attributes[i].as.expression->as.literal.kind == AST_LITERAL_INTEGER) {
                int val = atoi(el->attributes[i].as.expression->as.literal.as.text);
                if (strcmp(el->attributes[i].name, "width") == 0) w = val;
                if (strcmp(el->attributes[i].name, "height") == 0) h = val;
            }
        }
        fprintf(out, "    SolidGuiWindow *__jsx_%d = solid_bridge_window_new(%d, %d);\n",
                id, w, h);
    } else if (strcmp(el->tag_name, "Image") == 0) {
        int w = 100, h = 100, r = 0, g = 0, b = 0, a = 255;
        for (size_t i = 0; i < el->attr_count; i++) {
            if (el->attributes[i].kind == JSX_ATTR_EXPRESSION &&
                el->attributes[i].as.expression &&
                el->attributes[i].as.expression->kind == AST_EXPR_LITERAL &&
                el->attributes[i].as.expression->as.literal.kind == AST_LITERAL_INTEGER) {
                int val = atoi(el->attributes[i].as.expression->as.literal.as.text);
                if (strcmp(el->attributes[i].name, "width") == 0) w = val;
                if (strcmp(el->attributes[i].name, "height") == 0) h = val;
            }
            if (el->attributes[i].kind == JSX_ATTR_STYLE) {
                for (size_t j = 0; j < el->attributes[i].as.style.count; j++) {
                    JsxStyleProp *sp = &el->attributes[i].as.style.props[j];
                    if (strcmp(sp->name, "background-color") == 0 &&
                        sp->value && sp->value->kind == AST_EXPR_LITERAL &&
                        sp->value->as.literal.kind == AST_LITERAL_STRING) {
                        const char *cs = sp->value->as.literal.as.text;
                        if (cs[0] == '#' && strlen(cs + 1) >= 6) {
                            unsigned int cv;
                            sscanf(cs + 1, "%x", &cv);
                            if (strlen(cs + 1) == 8) {
                                r = (cv >> 24) & 0xFF; g = (cv >> 16) & 0xFF;
                                b = (cv >> 8) & 0xFF;  a = cv & 0xFF;
                            } else {
                                r = (cv >> 16) & 0xFF; g = (cv >> 8) & 0xFF;
                                b = cv & 0xFF; a = 255;
                            }
                        }
                    }
                }
            }
        }
        fprintf(out, "    SolidGuiImage *__jsx_%d = solid_bridge_image_new_color("
                "%d, %d, %d, %d, %d, %d);\n", id, w, h, r, g, b, a);
    } else if (strcmp(el->tag_name, "Text") == 0) {
        int size = 20, r = 255, g = 255, b = 255, a = 255;
        const char *text = "";
        for (size_t i = 0; i < el->attr_count; i++) {
            if (el->attributes[i].kind == JSX_ATTR_EXPRESSION &&
                el->attributes[i].as.expression &&
                el->attributes[i].as.expression->kind == AST_EXPR_LITERAL &&
                el->attributes[i].as.expression->as.literal.kind == AST_LITERAL_INTEGER) {
                int val = atoi(el->attributes[i].as.expression->as.literal.as.text);
                if (strcmp(el->attributes[i].name, "size") == 0) size = val;
            }
            if (el->attributes[i].kind == JSX_ATTR_STYLE) {
                for (size_t j = 0; j < el->attributes[i].as.style.count; j++) {
                    JsxStyleProp *sp = &el->attributes[i].as.style.props[j];
                    if (strcmp(sp->name, "color") == 0 &&
                        sp->value && sp->value->kind == AST_EXPR_LITERAL &&
                        sp->value->as.literal.kind == AST_LITERAL_STRING) {
                        const char *cs = sp->value->as.literal.as.text;
                        if (cs[0] == '#' && strlen(cs + 1) >= 6) {
                            unsigned int cv;
                            sscanf(cs + 1, "%x", &cv);
                            if (strlen(cs + 1) == 8) {
                                r = (cv >> 24) & 0xFF; g = (cv >> 16) & 0xFF;
                                b = (cv >> 8) & 0xFF;  a = cv & 0xFF;
                            } else {
                                r = (cv >> 16) & 0xFF; g = (cv >> 8) & 0xFF;
                                b = cv & 0xFF; a = 255;
                            }
                        }
                    }
                }
            }
        }
        /* Get text from first string child */
        for (size_t i = 0; i < el->child_count; i++) {
            if (el->children[i].kind == JSX_CHILD_TEXT) {
                text = el->children[i].as.text;
                break;
            }
            if (el->children[i].kind == JSX_CHILD_EXPRESSION &&
                el->children[i].as.expression &&
                el->children[i].as.expression->kind == AST_EXPR_LITERAL &&
                el->children[i].as.expression->as.literal.kind == AST_LITERAL_STRING) {
                text = el->children[i].as.expression->as.literal.as.text;
                break;
            }
        }
        fprintf(out, "    SolidGuiText *__jsx_%d = solid_bridge_text_new("
                "\"%s\", %d, %d, %d, %d, %d);\n", id, text, size, r, g, b, a);
    } else if (strcmp(el->tag_name, "Button") == 0) {
        int w = 100, h = 40;
        for (size_t i = 0; i < el->attr_count; i++) {
            if (el->attributes[i].kind == JSX_ATTR_EXPRESSION &&
                el->attributes[i].as.expression &&
                el->attributes[i].as.expression->kind == AST_EXPR_LITERAL &&
                el->attributes[i].as.expression->as.literal.kind == AST_LITERAL_INTEGER) {
                int val = atoi(el->attributes[i].as.expression->as.literal.as.text);
                if (strcmp(el->attributes[i].name, "width") == 0) w = val;
                if (strcmp(el->attributes[i].name, "height") == 0) h = val;
            }
        }
        fprintf(out, "    SolidGuiButton *__jsx_%d = solid_bridge_button_new(%d, %d);\n",
                id, w, h);
    } else if (strcmp(el->tag_name, "Pointer") == 0) {
        int channel = 0, size = 48;
        int r = 255, g = 255, b = 255, a = 204;
        for (size_t i = 0; i < el->attr_count; i++) {
            if (el->attributes[i].kind == JSX_ATTR_EXPRESSION &&
                el->attributes[i].as.expression &&
                el->attributes[i].as.expression->kind == AST_EXPR_LITERAL &&
                el->attributes[i].as.expression->as.literal.kind == AST_LITERAL_INTEGER) {
                int val = atoi(el->attributes[i].as.expression->as.literal.as.text);
                if (strcmp(el->attributes[i].name, "channel") == 0) channel = val;
                if (strcmp(el->attributes[i].name, "size") == 0) size = val;
            }
            if (el->attributes[i].kind == JSX_ATTR_STYLE) {
                for (size_t j = 0; j < el->attributes[i].as.style.count; j++) {
                    JsxStyleProp *sp = &el->attributes[i].as.style.props[j];
                    if (strcmp(sp->name, "color") == 0 &&
                        sp->value && sp->value->kind == AST_EXPR_LITERAL &&
                        sp->value->as.literal.kind == AST_LITERAL_STRING) {
                        const char *cs = sp->value->as.literal.as.text;
                        if (cs[0] == '#' && strlen(cs + 1) >= 6) {
                            unsigned int cv;
                            sscanf(cs + 1, "%x", &cv);
                            if (strlen(cs + 1) == 8) {
                                r = (cv >> 24) & 0xFF; g = (cv >> 16) & 0xFF;
                                b = (cv >> 8) & 0xFF;  a = cv & 0xFF;
                            } else {
                                r = (cv >> 16) & 0xFF; g = (cv >> 8) & 0xFF;
                                b = cv & 0xFF; a = 255;
                            }
                        }
                    }
                }
            }
        }
        fprintf(out, "    /* Pointer cursor — channel %d, size %d */\n", channel, size);
        fprintf(out, "    SolidGuiImage *__jsx_%d = solid_bridge_image_new_color("
                "%d, %d, %d, %d, %d, %d);\n", id, size, size, r, g, b, a);
        fprintf(out, "    solid_bridge_element_set_alignment_top_left("
                "solid_bridge_image_as_element(__jsx_%d));\n", id);
        fprintf(out, "    solid_register_pointer(%d, "
                "solid_bridge_image_as_element(__jsx_%d));\n", channel, id);
    } else {
        /* Unknown element — emit as generic */
        fprintf(out, "    SolidGuiElement *__jsx_%d = NULL; /* unknown: %s */\n",
                id, el->tag_name);
    }

    /* Position */
    {
        int x = -1, y = -1;
        for (size_t i = 0; i < el->attr_count; i++) {
            if (el->attributes[i].kind == JSX_ATTR_EXPRESSION &&
                el->attributes[i].as.expression &&
                el->attributes[i].as.expression->kind == AST_EXPR_LITERAL &&
                el->attributes[i].as.expression->as.literal.kind == AST_LITERAL_INTEGER) {
                int val = atoi(el->attributes[i].as.expression->as.literal.as.text);
                if (strcmp(el->attributes[i].name, "x") == 0) x = val;
                if (strcmp(el->attributes[i].name, "y") == 0) y = val;
            }
        }
        if (x >= 0 || y >= 0) {
            const char *as_el = NULL;
            if (strcmp(el->tag_name, "Text") == 0) as_el = "solid_bridge_text_as_element";
            else if (strcmp(el->tag_name, "Button") == 0) as_el = "solid_bridge_button_as_element";
            else if (strcmp(el->tag_name, "Image") == 0) as_el = "solid_bridge_image_as_element";
            if (as_el) {
                fprintf(out, "    solid_bridge_element_set_alignment_top_left("
                        "%s(__jsx_%d));\n",
                        as_el, id);
                fprintf(out, "    solid_bridge_element_set_position("
                        "%s(__jsx_%d), %d, %d);\n",
                        as_el, id, x >= 0 ? x : 0, y >= 0 ? y : 0);
            }
        }
    }

    /* onClick handler wiring */
    {
        int cur_handler = handler_idx;
        for (size_t i = 0; i < el->attr_count; i++) {
            if (el->attributes[i].kind == JSX_ATTR_HANDLER &&
                strcmp(el->attributes[i].name, "onClick") == 0 &&
                el->attributes[i].raw_text) {
                /* Find the handler in fctx->handlers */
                for (int h = 0; h < fctx->handler_count; h++) {
                    if (fctx->handlers[h].body_text == el->attributes[i].raw_text) {
                        fprintf(out, "    solid_bridge_button_set_click_handler(__jsx_%d, "
                                "__handler_%d, NULL);\n",
                                id, fctx->handlers[h].handler_id);
                        break;
                    }
                }
                cur_handler++;
            }
        }
    }

    /* Recurse children */
    for (size_t i = 0; i < el->child_count; i++) {
        if (el->children[i].kind == JSX_CHILD_ELEMENT) {
            int child_id = emit_jsx_element_full(fctx, el->children[i].as.element,
                                                  "", signals, signal_count,
                                                  id_counter, effect_counter,
                                                  handler_idx);

            /* Append child to parent */
            const char *child_as = NULL;
            const char *child_tag = el->children[i].as.element->tag_name;
            if (strcmp(child_tag, "Text") == 0) child_as = "solid_bridge_text_as_element";
            else if (strcmp(child_tag, "Button") == 0) child_as = "solid_bridge_button_as_element";
            else if (strcmp(child_tag, "Image") == 0) child_as = "solid_bridge_image_as_element";
            else if (strcmp(child_tag, "Pointer") == 0) child_as = "solid_bridge_image_as_element";

            if (strcmp(el->tag_name, "Window") == 0 && child_as) {
                fprintf(out, "    solid_bridge_window_append(__jsx_%d, "
                        "%s(__jsx_%d));\n", id, child_as, child_id);
            }

            /* Button label */
            if (strcmp(el->tag_name, "Button") == 0 &&
                strcmp(child_tag, "Text") == 0) {
                fprintf(out, "    solid_bridge_button_set_label(__jsx_%d, __jsx_%d);\n",
                        id, child_id);
            }
        }
    }

    /* Reactive text effects */
    for (size_t i = 0; i < el->child_count; i++) {
        if (el->children[i].kind == JSX_CHILD_EXPRESSION &&
            el->children[i].raw_text &&
            strcmp(el->tag_name, "Text") == 0) {
            const char *raw = el->children[i].raw_text;
            bool has_signal = false;
            for (int s = 0; s < signal_count; s++) {
                if (strstr(raw, signals[s].name)) { has_signal = true; break; }
            }
            if (has_signal) {
                int eid = (*effect_counter);
                fprintf(out, "    __eff_%d_ctx.text = __jsx_%d;\n", eid, id);
                fprintf(out, "    solid_create_effect(__effect_%d_fn, &__eff_%d_ctx);\n",
                        eid, eid);
                (*effect_counter)++;
            }
        }
    }

    return id;
}

/* ================================================================== */
/*  Component parsing + emission                                      */
/* ================================================================== */

static bool parse_and_emit_component(JsxFileContext *fctx, JsxTokenizer *jt)
{
    FILE *out = fctx->out;

    /* Already consumed TOK_COMPONENT. Next: name = () -> { */
    JsxToken name_tok = jsx_tokenizer_next(jt);
    if (name_tok.jsx_type != TOK_IDENTIFIER) return false;
    char *comp_name = jsx_dup_str(name_tok.start, name_tok.length);

    if (!jsx_expect(jt, TOK_ASSIGN)) { free(comp_name); return false; }
    if (!jsx_expect(jt, TOK_LPAREN)) { free(comp_name); return false; }
    if (!jsx_expect(jt, TOK_RPAREN)) { free(comp_name); return false; }
    if (!jsx_expect(jt, TOK_ARROW))  { free(comp_name); return false; }
    if (!jsx_expect(jt, TOK_LBRACE)) { free(comp_name); return false; }

    /* Parse signal declarations */
    SignalInfo signals[MAX_SIGNALS];
    int signal_count = 0;

    JsxToken tok = jsx_tokenizer_next(jt);
    while (tok.jsx_type == TOK_SIGNAL) {
        JsxToken sig_name = jsx_tokenizer_next(jt);
        if (sig_name.jsx_type != TOK_IDENTIFIER) break;

        JsxToken eq = jsx_tokenizer_next(jt);
        if (eq.jsx_type != TOK_ASSIGN) break;

        /* Collect initializer text until semicolon */
        const char *init_start = jt->base.current;
        JsxToken t;
        const char *init_end = init_start;
        do {
            t = jsx_tokenizer_next(jt);
            if (t.jsx_type == TOK_SEMICOLON || t.jsx_type == TOK_EOF) break;
            init_end = t.start + t.length;
        } while (1);

        /* Trim whitespace */
        while (init_start < init_end &&
               (*init_start == ' ' || *init_start == '\n')) init_start++;
        while (init_end > init_start &&
               (init_end[-1] == ' ' || init_end[-1] == '\n')) init_end--;

        if (signal_count < MAX_SIGNALS) {
            signals[signal_count].name = jsx_dup_str(sig_name.start, sig_name.length);
            signals[signal_count].init_text = jsx_dup_str(init_start,
                                                           (size_t)(init_end - init_start));
            signal_count++;
        }

        tok = jsx_tokenizer_next(jt);
    }

    /* Expect 'return' '(' then JSX element */
    if (tok.jsx_type != TOK_RETURN) {
        fprintf(stderr, "jsx: expected 'return' in component %s, got token type %d\n",
                comp_name, tok.jsx_type);
        free(comp_name);
        return false;
    }
    if (!jsx_expect(jt, TOK_LPAREN)) {
        fprintf(stderr, "jsx: expected '(' after return in component %s\n", comp_name);
        free(comp_name);
        return false;
    }

    /* Parse JSX root element */
    JsxElement *root = jsx_parse_element(jt);
    if (!root) {
        fprintf(stderr, "jsx: failed to parse JSX tree in component %s\n", comp_name);
        free(comp_name);
        return false;
    }

    /* Consume ); }; */
    jsx_expect(jt, TOK_RPAREN);    /* ) */
    jsx_expect(jt, TOK_SEMICOLON); /* ; */
    jsx_expect(jt, TOK_RBRACE);    /* } */
    jsx_expect(jt, TOK_SEMICOLON); /* ; */

    /* --- Emit signal globals (before effects/handlers that reference them) --- */
    for (int i = 0; i < signal_count; i++) {
        fprintf(out, "static SolidSignal *sig_%s;\n", signals[i].name);
    }
    fprintf(out, "\n");

    /* --- Emit reactive effect functions --- */
    int effect_pre_counter = 0;
    emit_text_effects(out, root, signals, signal_count, 0, &effect_pre_counter);
    /* recurse children for text effects */
    {
        int child_id = 1;
        for (size_t i = 0; i < root->child_count; i++) {
            if (root->children[i].kind == JSX_CHILD_ELEMENT) {
                emit_text_effects(out, root->children[i].as.element,
                                  signals, signal_count, child_id,
                                  &effect_pre_counter);
                child_id++;
            }
        }
    }

    /* --- Emit handler functions --- */
    fctx->handler_count = 0;
    extract_and_emit_handlers(fctx, root, signals, signal_count, 0);

    /* --- Emit component function --- */
    fprintf(out, "static SolidGuiWindow *__component_%s(void) {\n", comp_name);

    /* Init signals */
    for (int i = 0; i < signal_count; i++) {
        fprintf(out, "    sig_%s = solid_create_signal((CalyndaRtWord)(%s));\n",
                signals[i].name, signals[i].init_text);
    }
    fprintf(out, "\n");

    /* Emit element tree */
    int id_counter = 0;
    int effect_counter = 0;
    emit_jsx_element_full(fctx, root, "", signals, signal_count,
                          &id_counter, &effect_counter, 0);

    fprintf(out, "\n    return __jsx_0;\n");
    fprintf(out, "}\n\n");

    /* Store component info */
    if (fctx->component_count < MAX_COMPONENTS) {
        ComponentInfo *ci = &fctx->components[fctx->component_count++];
        ci->comp_name = comp_name;
        ci->root_element = root;
        ci->signal_count = signal_count;
        for (int i = 0; i < signal_count; i++) {
            ci->signals[i] = signals[i];
        }
    }

    return true;
}

/* ================================================================== */
/*  Boot function parsing + emission                                  */
/* ================================================================== */

static bool parse_and_emit_boot(JsxFileContext *fctx, JsxTokenizer *jt)
{
    FILE *out = fctx->out;

    /* Already consumed TOK_BOOT. Next: () -> { body }; */
    if (!jsx_expect(jt, TOK_LPAREN)) return false;
    if (!jsx_expect(jt, TOK_RPAREN)) return false;
    if (!jsx_expect(jt, TOK_ARROW))  return false;
    if (!jsx_expect(jt, TOK_LBRACE)) return false;

    fprintf(out, "static void __calynda_boot(void) {\n");

    /* Parse body tokens until matching } */
    /* We expect simple statements like gui.mount(Counter()); */
    JsxToken tok = jsx_tokenizer_next(jt);
    while (tok.jsx_type != TOK_RBRACE && tok.jsx_type != TOK_EOF) {
        /* Recognize gui.mount(Component()) pattern */
        if (tok.jsx_type == TOK_IDENTIFIER) {
            char *first = jsx_dup_str(tok.start, tok.length);
            JsxToken next = jsx_tokenizer_next(jt);

            if (next.jsx_type == TOK_DOT) {
                /* module.method(...) */
                JsxToken method = jsx_tokenizer_next(jt);
                if (method.jsx_type == TOK_IDENTIFIER) {
                    char *mname = jsx_dup_str(method.start, method.length);

                    if (strcmp(first, "gui") == 0 && strcmp(mname, "mount") == 0) {
                        /* gui.mount(expr) → solid_mount(expr) */
                        if (!jsx_expect(jt, TOK_LPAREN)) { free(first); free(mname); return false; }

                        /* Parse the argument — expect ComponentName() */
                        JsxToken arg = jsx_tokenizer_next(jt);
                        if (arg.jsx_type == TOK_IDENTIFIER) {
                            char *arg_name = jsx_dup_str(arg.start, arg.length);
                            JsxToken paren1 = jsx_tokenizer_next(jt);
                            if (paren1.jsx_type == TOK_LPAREN) {
                                jsx_expect(jt, TOK_RPAREN); /* () */
                            }
                            fprintf(out, "    SolidGuiWindow *__root = __component_%s();\n",
                                    arg_name);
                            fprintf(out, "    solid_mount(__root);\n");
                            free(arg_name);
                        }
                        jsx_expect(jt, TOK_RPAREN); /* close gui.mount() */
                        jsx_expect(jt, TOK_SEMICOLON);
                    }
                    free(mname);
                }
            }
            free(first);
        }

        tok = jsx_tokenizer_next(jt);
    }

    fprintf(out, "}\n\n");

    /* Consume trailing ; */
    jsx_expect(jt, TOK_SEMICOLON);

    return true;
}

/* ================================================================== */
/*  Public API                                                        */
/* ================================================================== */

bool jsx_source_has_components(const char *source)
{
    /* Quick scan for "component" keyword followed by an identifier */
    const char *p = source;
    while ((p = strstr(p, "component")) != NULL) {
        /* Check it's at a word boundary */
        if (p > source && (isalnum((unsigned char)p[-1]) || p[-1] == '_')) {
            p += 9;
            continue;
        }
        if (isalnum((unsigned char)p[9]) && p[9] != ' ') {
            p += 9;
            continue;
        }
        return true;
    }
    return false;
}

int jsx_compile_to_c(const char *source, const char *path, FILE *out)
{
    JsxFileContext fctx;
    JsxTokenizer jt;

    memset(&fctx, 0, sizeof(fctx));
    fctx.out = out;

    jsx_tokenizer_init(&jt, source);

    /* File header */
    fprintf(out, "/* Generated by Calynda JSX compiler from %s */\n\n", path);
    fprintf(out, "#include \"calynda_runtime.h\"\n");
    fprintf(out, "#include \"solid_runtime.h\"\n");
    fprintf(out, "#include <stdio.h>\n");
    fprintf(out, "#include <string.h>\n\n");

    /* Walk tokens */
    JsxToken tok;
    while (1) {
        tok = jsx_tokenizer_next(&jt);
        if (tok.jsx_type == TOK_EOF) break;

        if (tok.jsx_type == TOK_IMPORT) {
            /* Skip import declarations — already emitted includes */
            jsx_skip_to_semicolon(&jt);
        }
        else if (tok.jsx_type == TOK_COMPONENT) {
            if (!parse_and_emit_component(&fctx, &jt)) {
                fprintf(stderr, "%s: jsx: failed to compile component\n", path);
                return 1;
            }
        }
        else if (tok.jsx_type == TOK_BOOT) {
            if (!parse_and_emit_boot(&fctx, &jt)) {
                fprintf(stderr, "%s: jsx: failed to compile boot function\n", path);
                return 1;
            }
        }
        /* Ignore other tokens (comments in between) */
    }

    /* Emit main() — solid_mount() is the event loop and does not return */
    fprintf(out, "int main(int argc, char **argv) {\n");
    fprintf(out, "    (void)argc; (void)argv;\n");
    fprintf(out, "    calynda_rt_init();\n");
    fprintf(out, "    __calynda_boot();\n");
    fprintf(out, "    return 0;\n");
    fprintf(out, "}\n");

    /* Cleanup */
    for (int i = 0; i < fctx.component_count; i++) {
        free(fctx.components[i].comp_name);
        if (fctx.components[i].root_element)
            jsx_element_free(fctx.components[i].root_element);
        for (int j = 0; j < fctx.components[i].signal_count; j++) {
            free(fctx.components[i].signals[j].name);
            free(fctx.components[i].signals[j].init_text);
        }
    }

    return 0;
}
