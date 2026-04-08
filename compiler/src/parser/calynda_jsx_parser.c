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
 * Also captures the raw source text between { and } into raw_text_out
 * (if non-NULL) for use at emit time.
 */
static AstExpression *jsx_parse_inline_expression(JsxTokenizer *jt,
                                                  char **raw_text_out)
{
    /* Record start position (right after the opening {) */
    const char *raw_start = jt->base.current;

    AstExpression *expr = (AstExpression *)calloc(1, sizeof(AstExpression));
    if (!expr) {
        if (raw_text_out) *raw_text_out = NULL;
        return NULL;
    }

    JsxToken tok = jsx_tokenizer_next(jt);
    const char *last_end = tok.start + tok.length;

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
    JsxToken next = jsx_tokenizer_next(jt);
    while (next.jsx_type != TOK_JSX_EXPR_END &&
           next.jsx_type != TOK_EOF) {
        last_end = next.start + next.length;
        next = jsx_tokenizer_next(jt);
    }

    /* Capture raw text from start to just before the closing } */
    if (raw_text_out) {
        const char *raw_end = next.start; /* points at } */
        /* Trim whitespace */
        while (raw_end > raw_start && (raw_end[-1] == ' ' || raw_end[-1] == '\n' ||
               raw_end[-1] == '\r' || raw_end[-1] == '\t')) raw_end--;
        while (raw_start < raw_end && (*raw_start == ' ' || *raw_start == '\n' ||
               *raw_start == '\r' || *raw_start == '\t')) raw_start++;
        *raw_text_out = dup_str(raw_start, (size_t)(raw_end - raw_start));
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

/* ---- CSS string parsing (Solite-style) ---- */

/*
 * Parse a CSS string like "color: #FFFFFF; background-color: #2D2D2D;"
 * into JsxStyleProp entries on the attribute.
 */
static void jsx_parse_css_string(const char *css, JsxAttribute *attr)
{
    const char *p = css;

    while (*p) {
        /* Skip whitespace */
        while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') p++;
        if (*p == '\0') break;

        /* Property name (up to ':') */
        const char *name_start = p;
        while (*p && *p != ':' && *p != ';') p++;
        if (*p != ':') break;
        const char *name_end = p;
        /* Trim trailing whitespace from name */
        while (name_end > name_start &&
               (name_end[-1] == ' ' || name_end[-1] == '\t')) name_end--;
        p++; /* skip ':' */

        /* Skip whitespace */
        while (*p == ' ' || *p == '\t') p++;

        /* Property value (up to ';' or end) */
        const char *val_start = p;
        while (*p && *p != ';') p++;
        const char *val_end = p;
        /* Trim trailing whitespace from value */
        while (val_end > val_start &&
               (val_end[-1] == ' ' || val_end[-1] == '\t')) val_end--;
        if (*p == ';') p++;

        if (name_end <= name_start || val_end <= val_start) continue;

        /* Build the style prop */
        AstExpression *val_expr = (AstExpression *)calloc(1, sizeof(AstExpression));
        if (!val_expr) continue;

        /* Check if it looks like a number */
        bool is_number = true;
        for (const char *q = val_start; q < val_end; q++) {
            if (!isdigit((unsigned char)*q) && *q != '.' && *q != '-') {
                is_number = false;
                break;
            }
        }

        if (is_number && val_end > val_start) {
            val_expr->kind = AST_EXPR_LITERAL;
            val_expr->as.literal.kind = AST_LITERAL_INTEGER;
            val_expr->as.literal.as.text = dup_str(val_start,
                                                    (size_t)(val_end - val_start));
        } else {
            val_expr->kind = AST_EXPR_LITERAL;
            val_expr->as.literal.kind = AST_LITERAL_STRING;
            val_expr->as.literal.as.text = dup_str(val_start,
                                                    (size_t)(val_end - val_start));
        }

        /* Grow array if needed */
        if (attr->as.style.count >= attr->as.style.capacity) {
            size_t new_cap = attr->as.style.capacity * 2;
            JsxStyleProp *new_props = (JsxStyleProp *)realloc(
                attr->as.style.props, sizeof(JsxStyleProp) * new_cap);
            if (!new_props) { free(val_expr); continue; }
            attr->as.style.props = new_props;
            attr->as.style.capacity = new_cap;
        }

        JsxStyleProp *prop = &attr->as.style.props[attr->as.style.count++];
        prop->name = dup_str(name_start, (size_t)(name_end - name_start));
        prop->value = val_expr;
    }
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
            if (strcmp(attr_name, "style") == 0) {
                /* Solite-style string notation: style="color: #FFF; width: 100px;" */
                char *css_str = (val.length >= 2)
                    ? dup_str(val.start + 1, val.length - 2) : dup_str("", 0);
                JsxAttribute attr;
                memset(&attr, 0, sizeof(attr));
                attr.name = attr_name;
                attr.kind = JSX_ATTR_STYLE;
                attr.as.style.count = 0;
                attr.as.style.capacity = 4;
                attr.as.style.props = (JsxStyleProp *)calloc(4, sizeof(JsxStyleProp));
                if (attr.as.style.props && css_str) {
                    jsx_parse_css_string(css_str, &attr);
                }
                free(css_str);
                jsx_element_add_attr(element, attr);
            } else {
                /* String attribute: name="value" */
                JsxAttribute attr = jsx_attr_string(
                    attr_name,
                    val.length >= 2 ? dup_str(val.start + 1, val.length - 2) : "");
                free(attr_name);
                jsx_element_add_attr(element, attr);
            }
        } else if (val.jsx_type == TOK_JSX_EXPR_START) {
            /* Check for style={{ }} — keep for backwards compat */
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
                char *raw = NULL;
                AstExpression *handler = jsx_parse_inline_expression(jt, &raw);
                JsxAttribute attr = jsx_attr_handler(attr_name, handler);
                attr.raw_text = raw;
                free(attr_name);
                jsx_element_add_attr(element, attr);
            } else {
                /* Expression attribute: name={expr} */
                char *raw = NULL;
                AstExpression *expr = jsx_parse_inline_expression(jt, &raw);
                JsxAttribute attr = jsx_attr_expr(attr_name, expr);
                attr.raw_text = raw;
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
            char *raw = NULL;
            AstExpression *expr = jsx_parse_inline_expression(jt, &raw);
            if (expr) {
                JsxChild child = jsx_child_expr(expr);
                child.raw_text = raw;
                jsx_element_add_child(element, child);
            } else {
                free(raw);
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
