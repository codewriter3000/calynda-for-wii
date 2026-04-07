/*
 * calynda_jsx_parser.c — JSX parser implementation.
 *
 * Parses JSX elements by consuming JsxTokens from the JsxTokenizer.
 * JSX elements are parsed recursively to support arbitrary nesting.
 *
 * Grammar (simplified):
 *   JsxElement   ::= '<' TagName Attributes '>' Children '</' TagName '>'
 *                   | '<' TagName Attributes '/>'
 *   Attributes   ::= (Name '=' Value)*
 *   Value        ::= StringLiteral | '{' Expression '}'
 *   Children     ::= (JsxElement | '{' Expression '}' | Text)*
 *   Style        ::= 'style' '=' '{{' StyleProps '}}'
 *   StyleProps   ::= (CssName ':' Expression ','?)*
 */

#include "calynda_jsx_parser.h"
#include "calynda_jsx_tokens.h"
#include "calynda_jsx_ast.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* ---- Helpers ---- */

static char *dup_str(const char *s, size_t len)
{
    char *d = (char *)malloc(len + 1);
    if (d) {
        memcpy(d, s, len);
        d[len] = '\0';
    }
    return d;
}

/*
 * Parse a single expression from the JSX tokenizer.
 * We re-use the base tokenizer to get standard Calynda tokens
 * until we hit the closing '}' at depth 0.
 *
 * For now, this creates a simple literal or identifier expression
 * from the token stream. A full implementation would invoke the
 * Calynda parser's parse_expression_node().
 */
static AstExpression *jsx_parse_inline_expression(JsxTokenizer *jt)
{
    /* Collect tokens until JSX_EXPR_END or matching '}' */
    /* For the initial implementation, handle simple cases:
       - integer literal
       - string literal
       - identifier (variable reference)
       - template literal
       - lambda expression: () -> expr
    */

    AstExpression *expr = (AstExpression *)calloc(1, sizeof(AstExpression));
    if (!expr) return NULL;

    JsxToken tok = jsx_tokenizer_next(jt);

    if (tok.jsx_type == TOK_INT_LIT) {
        expr->kind = AST_EXPR_LITERAL;
        expr->as.literal.kind = AST_LITERAL_INTEGER;
        expr->as.literal.as.text = dup_str(tok.start, tok.length);
    } else if (tok.jsx_type == TOK_STRING_LIT) {
        expr->kind = AST_EXPR_LITERAL;
        expr->as.literal.kind = AST_LITERAL_STRING;
        /* Strip quotes */
        if (tok.length >= 2) {
            expr->as.literal.as.text = dup_str(tok.start + 1, tok.length - 2);
        } else {
            expr->as.literal.as.text = dup_str(tok.start, tok.length);
        }
    } else if (tok.jsx_type == TOK_IDENTIFIER) {
        expr->kind = AST_EXPR_IDENTIFIER;
        expr->as.identifier = dup_str(tok.start, tok.length);
    } else if (tok.jsx_type == TOK_TEMPLATE_START || tok.jsx_type == TOK_TEMPLATE_FULL) {
        expr->kind = AST_EXPR_LITERAL;
        expr->as.literal.kind = AST_LITERAL_TEMPLATE;
        /* Template handling is complex — for now store as string */
        expr->as.literal.as.text = dup_str(tok.start, tok.length);
    } else if (tok.jsx_type == TOK_TRUE) {
        expr->kind = AST_EXPR_LITERAL;
        expr->as.literal.kind = AST_LITERAL_BOOL;
        expr->as.literal.as.bool_value = true;
    } else if (tok.jsx_type == TOK_FALSE) {
        expr->kind = AST_EXPR_LITERAL;
        expr->as.literal.kind = AST_LITERAL_BOOL;
        expr->as.literal.as.bool_value = false;
    } else {
        /* Fallback: treat as identifier */
        expr->kind = AST_EXPR_IDENTIFIER;
        expr->as.identifier = dup_str(tok.start, tok.length);
    }

    /* Consume remaining tokens until we hit EXPR_END */
    /* A proper implementation chains binary/call/member exprs here */
    JsxToken next = jsx_tokenizer_next(jt);
    while (next.jsx_type != TOK_JSX_EXPR_END &&
           next.jsx_type != TOK_EOF) {
        /* Skip tokens in complex expressions for now */
        next = jsx_tokenizer_next(jt);
    }

    return expr;
}

/* ---- Style parsing ---- */

bool jsx_parse_style_object(JsxTokenizer *jt, JsxAttribute *attr)
{
    /* At this point we've consumed style={ and expect { props } } */
    attr->kind = JSX_ATTR_STYLE;
    attr->as.style.count = 0;
    attr->as.style.capacity = 4;
    attr->as.style.props = (JsxStyleProp *)calloc(4, sizeof(JsxStyleProp));
    if (!attr->as.style.props) return false;

    /* Expect opening { of the style object */
    JsxToken tok = jsx_tokenizer_next(jt);
    if (tok.jsx_type != TOK_LBRACE) return false;

    /* Parse property: value pairs */
    while (true) {
        tok = jsx_tokenizer_next(jt);

        /* End of style object } */
        if (tok.jsx_type == TOK_RBRACE) break;
        if (tok.jsx_type == TOK_EOF) return false;

        /* Property name (may contain hyphens, so scan as raw text) */
        if (tok.jsx_type != TOK_IDENTIFIER && tok.jsx_type != TOK_STRING_LIT)
            return false;

        /* Build property name — handle hyphenated names like background-color */
        char prop_name[64];
        size_t name_len = tok.length < 63 ? tok.length : 63;
        memcpy(prop_name, tok.start, name_len);
        prop_name[name_len] = '\0';

        /* Check for hyphenated continuation (e.g. background-color) */
        JsxToken peek = jsx_tokenizer_next(jt);
        while (peek.jsx_type == TOK_MINUS) {
            /* Append '-' + next identifier */
            size_t cur_len = strlen(prop_name);
            if (cur_len < 62) {
                prop_name[cur_len] = '-';
                prop_name[cur_len + 1] = '\0';
            }
            JsxToken id_part = jsx_tokenizer_next(jt);
            if (id_part.jsx_type == TOK_IDENTIFIER) {
                size_t cl = strlen(prop_name);
                size_t add = id_part.length < (63 - cl) ? id_part.length : (63 - cl);
                memcpy(prop_name + cl, id_part.start, add);
                prop_name[cl + add] = '\0';
            }
            peek = jsx_tokenizer_next(jt);
        }

        /* peek should be ':' */
        if (peek.jsx_type != TOK_COLON) return false;

        /* Parse value expression */
        /* For color values like #FF0000, handle specially */
        JsxToken val_tok = jsx_tokenizer_next(jt);
        AstExpression *val_expr = (AstExpression *)calloc(1, sizeof(AstExpression));
        if (!val_expr) return false;

        if (val_tok.jsx_type == TOK_INT_LIT) {
            val_expr->kind = AST_EXPR_LITERAL;
            val_expr->as.literal.kind = AST_LITERAL_INTEGER;
            val_expr->as.literal.as.text = dup_str(val_tok.start, val_tok.length);
        } else if (val_tok.jsx_type == TOK_STRING_LIT) {
            val_expr->kind = AST_EXPR_LITERAL;
            val_expr->as.literal.kind = AST_LITERAL_STRING;
            if (val_tok.length >= 2) {
                val_expr->as.literal.as.text = dup_str(val_tok.start + 1, val_tok.length - 2);
            } else {
                val_expr->as.literal.as.text = dup_str(val_tok.start, val_tok.length);
            }
        } else if (val_tok.jsx_type == TOK_IDENTIFIER) {
            val_expr->kind = AST_EXPR_IDENTIFIER;
            val_expr->as.identifier = dup_str(val_tok.start, val_tok.length);
        } else {
            val_expr->kind = AST_EXPR_LITERAL;
            val_expr->as.literal.kind = AST_LITERAL_INTEGER;
            val_expr->as.literal.as.text = dup_str(val_tok.start, val_tok.length);
        }

        /* Grow style array if needed */
        if (attr->as.style.count >= attr->as.style.capacity) {
            size_t new_cap = attr->as.style.capacity * 2;
            JsxStyleProp *new_props = (JsxStyleProp *)realloc(
                attr->as.style.props, sizeof(JsxStyleProp) * new_cap);
            if (!new_props) { free(val_expr); return false; }
            attr->as.style.props = new_props;
            attr->as.style.capacity = new_cap;
        }

        JsxStyleProp *prop = &attr->as.style.props[attr->as.style.count++];
        prop->name = dup_str(prop_name, strlen(prop_name));
        prop->value = val_expr;

        /* Optional comma */
        tok = jsx_tokenizer_next(jt);
        if (tok.jsx_type == TOK_RBRACE) break;
        /* If not comma or }, something is wrong but we continue */
        if (tok.jsx_type != TOK_COMMA) {
            /* Push back / handle */
        }
    }

    return true;
}

/* ---- Attribute parsing ---- */

bool jsx_parse_attributes(JsxTokenizer *jt, JsxElement *element)
{
    while (true) {
        JsxToken tok = jsx_tokenizer_next(jt);

        /* End of attributes */
        if (tok.jsx_type == TOK_JSX_TAG_END) {
            return true;
        }
        if (tok.jsx_type == TOK_JSX_SELF_CLOSE) {
            element->self_closing = true;
            return true;
        }
        if (tok.jsx_type == TOK_EOF) return false;

        /* Attribute name */
        if (tok.jsx_type != TOK_IDENTIFIER) continue;

        char *attr_name = dup_str(tok.start, tok.length);

        /* Expect '=' */
        JsxToken eq = jsx_tokenizer_next(jt);
        if (eq.jsx_type != TOK_ASSIGN) {
            /* Boolean attribute (no value) */
            JsxAttribute attr;
            memset(&attr, 0, sizeof(attr));
            attr.name = attr_name;
            attr.kind = JSX_ATTR_EXPRESSION;
            /* Create a `true` literal */
            AstExpression *true_expr = (AstExpression *)calloc(1, sizeof(AstExpression));
            if (true_expr) {
                true_expr->kind = AST_EXPR_LITERAL;
                true_expr->as.literal.kind = AST_LITERAL_BOOL;
                true_expr->as.literal.as.bool_value = true;
            }
            attr.as.expression = true_expr;
            jsx_element_add_attr(element, attr);
            /* We need to handle the token we just consumed (eq) */
            continue;
        }

        /* Attribute value */
        JsxToken val = jsx_tokenizer_next(jt);

        if (val.jsx_type == TOK_STRING_LIT) {
            /* String attribute: name="value" */
            JsxAttribute attr = jsx_attr_string(
                attr_name,
                val.length >= 2 ? dup_str(val.start + 1, val.length - 2) : "");
            free(attr_name);
            jsx_element_add_attr(element, attr);
        } else if (val.jsx_type == TOK_JSX_EXPR_START) {
            /* Check for style={{ }} */
            if (strcmp(attr_name, "style") == 0) {
                JsxAttribute attr;
                memset(&attr, 0, sizeof(attr));
                attr.name = attr_name;
                if (!jsx_parse_style_object(jt, &attr)) {
                    free(attr_name);
                    return false;
                }
                /* Consume the outer closing } (JSX_EXPR_END) */
                JsxToken end = jsx_tokenizer_next(jt);
                (void)end;
                jsx_element_add_attr(element, attr);
            } else if (strncmp(attr_name, "on", 2) == 0) {
                /* Event handler: onClick={() -> expr} */
                AstExpression *handler = jsx_parse_inline_expression(jt);
                JsxAttribute attr = jsx_attr_handler(attr_name, handler);
                free(attr_name);
                jsx_element_add_attr(element, attr);
            } else {
                /* Expression attribute: name={expr} */
                AstExpression *expr = jsx_parse_inline_expression(jt);
                JsxAttribute attr = jsx_attr_expr(attr_name, expr);
                free(attr_name);
                jsx_element_add_attr(element, attr);
            }
        } else if (val.jsx_type == TOK_INT_LIT) {
            /* Numeric as expression */
            AstExpression *expr = (AstExpression *)calloc(1, sizeof(AstExpression));
            if (expr) {
                expr->kind = AST_EXPR_LITERAL;
                expr->as.literal.kind = AST_LITERAL_INTEGER;
                expr->as.literal.as.text = dup_str(val.start, val.length);
            }
            JsxAttribute attr = jsx_attr_expr(attr_name, expr);
            free(attr_name);
            jsx_element_add_attr(element, attr);
        } else {
            free(attr_name);
        }
    }
}

/* ---- Children parsing ---- */

bool jsx_parse_children(JsxTokenizer *jt, JsxElement *element)
{
    while (true) {
        JsxToken tok = jsx_tokenizer_next(jt);

        if (tok.jsx_type == TOK_EOF) return false;

        /* Closing tag </Name> */
        if (tok.jsx_type == TOK_JSX_CLOSE_START) {
            /* Verify tag name matches */
            if (element->tag_name && tok.tag_name[0] != '\0') {
                if (strcmp(element->tag_name, tok.tag_name) != 0) {
                    /* Mismatched closing tag */
                    return false;
                }
            }
            return true;
        }

        /* Nested element */
        if (tok.jsx_type == TOK_JSX_OPEN_START) {
            /* We got a nested <Tag — parse recursively.
               We need to back-track the tokenizer state slightly:
               create a child element with the tag_name from tok. */
            JsxElement *child = jsx_element_new();
            if (!child) return false;
            child->tag_name = dup_str(tok.tag_name, strlen(tok.tag_name));
            child->is_component = (child->tag_name[0] >= 'A' && child->tag_name[0] <= 'Z');
            child->source_span.start_line = tok.line;
            child->source_span.start_column = tok.column;

            /* Parse attributes */
            if (!jsx_parse_attributes(jt, child)) {
                jsx_element_free(child);
                return false;
            }

            /* Parse children if not self-closing */
            if (!child->self_closing) {
                if (!jsx_parse_children(jt, child)) {
                    jsx_element_free(child);
                    return false;
                }
            }

            jsx_element_add_child(element, jsx_child_element(child));
        }

        /* Expression child {expr} */
        else if (tok.jsx_type == TOK_JSX_EXPR_START) {
            AstExpression *expr = jsx_parse_inline_expression(jt);
            if (expr) {
                jsx_element_add_child(element, jsx_child_expr(expr));
            }
        }

        /* Text child */
        else if (tok.jsx_type == TOK_JSX_TEXT) {
            /* Trim whitespace-only text */
            bool all_space = true;
            for (size_t i = 0; i < tok.length; i++) {
                if (tok.start[i] != ' ' && tok.start[i] != '\t' &&
                    tok.start[i] != '\n' && tok.start[i] != '\r') {
                    all_space = false;
                    break;
                }
            }
            if (!all_space) {
                char *text = dup_str(tok.start, tok.length);
                if (text) {
                    jsx_element_add_child(element, jsx_child_text(text));
                    free(text); /* jsx_child_text makes its own copy */
                }
            }
        }
    }
}

/* ---- Main element parser ---- */

JsxElement *jsx_parse_element(JsxTokenizer *jt)
{
    /* Expect TOK_JSX_OPEN_START to have been consumed already,
       or consume it now */
    JsxToken tok = jsx_tokenizer_next(jt);
    if (tok.jsx_type != TOK_JSX_OPEN_START) return NULL;

    JsxElement *el = jsx_element_new();
    if (!el) return NULL;

    el->tag_name = dup_str(tok.tag_name, strlen(tok.tag_name));
    el->is_component = (el->tag_name[0] >= 'A' && el->tag_name[0] <= 'Z');
    el->source_span.start_line = tok.line;
    el->source_span.start_column = tok.column;

    /* Parse attributes */
    if (!jsx_parse_attributes(jt, el)) {
        jsx_element_free(el);
        return NULL;
    }

    /* Parse children if not self-closing */
    if (!el->self_closing) {
        if (!jsx_parse_children(jt, el)) {
            jsx_element_free(el);
            return NULL;
        }
    }

    return el;
}
