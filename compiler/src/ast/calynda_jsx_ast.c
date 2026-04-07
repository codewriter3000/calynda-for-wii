/*
 * calynda_jsx_ast.c — JSX AST node construction and destruction.
 */

#include "calynda_jsx_ast.h"

#include <stdlib.h>
#include <string.h>

/* ---- Element ---- */

JsxElement *jsx_element_new(void)
{
    JsxElement *el = (JsxElement *)calloc(1, sizeof(JsxElement));
    if (!el) return NULL;
    el->attr_capacity = 4;
    el->attributes = (JsxAttribute *)calloc(el->attr_capacity, sizeof(JsxAttribute));
    el->child_capacity = 4;
    el->children = (JsxChild *)calloc(el->child_capacity, sizeof(JsxChild));
    return el;
}

void jsx_element_free(JsxElement *el)
{
    if (!el) return;
    free(el->tag_name);

    for (size_t i = 0; i < el->attr_count; i++) {
        free(el->attributes[i].name);
        if (el->attributes[i].kind == JSX_ATTR_STRING) {
            free(el->attributes[i].as.string_val);
        }
        if (el->attributes[i].kind == JSX_ATTR_STYLE) {
            for (size_t j = 0; j < el->attributes[i].as.style.count; j++) {
                free(el->attributes[i].as.style.props[j].name);
            }
            free(el->attributes[i].as.style.props);
        }
    }
    free(el->attributes);

    for (size_t i = 0; i < el->child_count; i++) {
        if (el->children[i].kind == JSX_CHILD_ELEMENT) {
            jsx_element_free(el->children[i].as.element);
        } else if (el->children[i].kind == JSX_CHILD_TEXT) {
            free(el->children[i].as.text);
        }
    }
    free(el->children);

    free(el);
}

/* ---- Add attribute ---- */

void jsx_element_add_attr(JsxElement *el, JsxAttribute attr)
{
    if (!el) return;

    if (el->attr_count >= el->attr_capacity) {
        size_t new_cap = el->attr_capacity * 2;
        JsxAttribute *new_attrs = (JsxAttribute *)realloc(
            el->attributes, sizeof(JsxAttribute) * new_cap);
        if (!new_attrs) return;
        el->attributes = new_attrs;
        el->attr_capacity = new_cap;
    }

    el->attributes[el->attr_count++] = attr;
}

/* ---- Add child ---- */

void jsx_element_add_child(JsxElement *el, JsxChild child)
{
    if (!el) return;

    if (el->child_count >= el->child_capacity) {
        size_t new_cap = el->child_capacity * 2;
        JsxChild *new_children = (JsxChild *)realloc(
            el->children, sizeof(JsxChild) * new_cap);
        if (!new_children) return;
        el->children = new_children;
        el->child_capacity = new_cap;
    }

    el->children[el->child_count++] = child;
}

/* ---- Attribute constructors ---- */

static char *str_dup(const char *s)
{
    if (!s) return NULL;
    size_t len = strlen(s);
    char *d = (char *)malloc(len + 1);
    if (d) memcpy(d, s, len + 1);
    return d;
}

JsxAttribute jsx_attr_string(const char *name, const char *value)
{
    JsxAttribute attr;
    memset(&attr, 0, sizeof(attr));
    attr.name = str_dup(name);
    attr.kind = JSX_ATTR_STRING;
    attr.as.string_val = str_dup(value);
    return attr;
}

JsxAttribute jsx_attr_expr(const char *name, AstExpression *expr)
{
    JsxAttribute attr;
    memset(&attr, 0, sizeof(attr));
    attr.name = str_dup(name);
    attr.kind = JSX_ATTR_EXPRESSION;
    attr.as.expression = expr;
    return attr;
}

JsxAttribute jsx_attr_handler(const char *name, AstExpression *lambda)
{
    JsxAttribute attr;
    memset(&attr, 0, sizeof(attr));
    attr.name = str_dup(name);
    attr.kind = JSX_ATTR_HANDLER;
    attr.as.expression = lambda;
    return attr;
}

/* ---- Child constructors ---- */

JsxChild jsx_child_element(JsxElement *el)
{
    JsxChild child;
    memset(&child, 0, sizeof(child));
    child.kind = JSX_CHILD_ELEMENT;
    child.as.element = el;
    return child;
}

JsxChild jsx_child_text(const char *text)
{
    JsxChild child;
    memset(&child, 0, sizeof(child));
    child.kind = JSX_CHILD_TEXT;
    child.as.text = str_dup(text);
    return child;
}

JsxChild jsx_child_expr(AstExpression *expr)
{
    JsxChild child;
    memset(&child, 0, sizeof(child));
    child.kind = JSX_CHILD_EXPRESSION;
    child.as.expression = expr;
    return child;
}
