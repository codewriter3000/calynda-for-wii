#ifndef CALYNDA_JSX_AST_H
#define CALYNDA_JSX_AST_H

/*
 * calynda_jsx_ast.h — AST node types for JSX elements.
 *
 * These extend the Calynda AST with JSX-specific nodes that represent
 * the declarative UI tree. They are later lowered in HIR to imperative
 * libwiigui bridge calls.
 */

#include "ast.h"
#include "ast_types.h"
#include "ast_decl_types.h"

#include <stdbool.h>
#include <stddef.h>

/* ---- JSX attribute ---- */

typedef enum {
    JSX_ATTR_STRING,      /* name="value"           */
    JSX_ATTR_EXPRESSION,  /* name={expr}            */
    JSX_ATTR_STYLE,       /* style={{ key: val }}   */
    JSX_ATTR_HANDLER      /* onClick={() -> expr}   */
} JsxAttrKind;

typedef struct {
    char          *name;         /* attribute name (e.g. "width", "onClick") */
    JsxAttrKind    kind;
    union {
        char          *string_val;    /* JSX_ATTR_STRING  */
        AstExpression *expression;    /* JSX_ATTR_EXPRESSION / JSX_ATTR_HANDLER */
        struct {
            JsxStyleProp *props;
            size_t        count;
            size_t        capacity;
        } style;                      /* JSX_ATTR_STYLE   */
    } as;
    AstSourceSpan  source_span;
} JsxAttribute;

/* ---- JSX style property (within style={{ ... }}) ---- */

typedef struct JsxStyleProp {
    char          *name;         /* CSS property name (e.g. "background-color") */
    AstExpression *value;        /* value expression */
    AstSourceSpan  source_span;
} JsxStyleProp;

/* ---- JSX child ---- */

typedef enum {
    JSX_CHILD_ELEMENT,     /* nested <Element> */
    JSX_CHILD_TEXT,        /* plain text       */
    JSX_CHILD_EXPRESSION   /* {expr}           */
} JsxChildKind;

typedef struct JsxChild {
    JsxChildKind kind;
    union {
        struct JsxElement *element;      /* JSX_CHILD_ELEMENT    */
        char              *text;         /* JSX_CHILD_TEXT       */
        AstExpression     *expression;   /* JSX_CHILD_EXPRESSION */
    } as;
    AstSourceSpan source_span;
} JsxChild;

/* ---- JSX element ---- */

typedef struct JsxElement {
    char          *tag_name;      /* e.g. "Window", "Button", "Text" */
    bool           is_component;  /* true if user-defined component  */
    bool           self_closing;  /* true if <Foo /> */

    JsxAttribute  *attributes;
    size_t         attr_count;
    size_t         attr_capacity;

    JsxChild      *children;
    size_t         child_count;
    size_t         child_capacity;

    AstSourceSpan  source_span;
} JsxElement;

/* ---- JSX component declaration ---- */

/*
 * component Counter(props) -> { ... return <Window>...</Window> }
 *
 * Represented as:
 *   AstTopLevelDecl with kind = AST_TOP_LEVEL_COMPONENT
 *   containing a JsxComponentDecl
 */
typedef struct {
    char              *name;
    AstParameterList   parameters;
    AstLambdaBody      body;          /* function body */
    AstSourceSpan      name_span;
} JsxComponentDecl;

/* ---- JSX signal declaration ---- */

/*
 * signal count = 0
 *
 * Desugars to: createSignal(0) returning [getter, setter]
 */
typedef struct {
    char          *name;
    AstExpression *initializer;
    AstSourceSpan  name_span;
} JsxSignalDecl;

/* ---- Construction / destruction ---- */

JsxElement   *jsx_element_new(void);
void          jsx_element_free(JsxElement *el);

void          jsx_element_add_attr(JsxElement *el, JsxAttribute attr);
void          jsx_element_add_child(JsxElement *el, JsxChild child);

JsxAttribute  jsx_attr_string(const char *name, const char *value);
JsxAttribute  jsx_attr_expr(const char *name, AstExpression *expr);
JsxAttribute  jsx_attr_handler(const char *name, AstExpression *lambda);

JsxChild      jsx_child_element(JsxElement *el);
JsxChild      jsx_child_text(const char *text);
JsxChild      jsx_child_expr(AstExpression *expr);

#endif /* CALYNDA_JSX_AST_H */
