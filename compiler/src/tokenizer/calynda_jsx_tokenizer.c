/*
 * calynda_jsx_tokenizer.c — JSX-aware tokenizer layer.
 *
 * Wraps the standard Calynda tokenizer and adds JSX context tracking.
 * When '<' is followed by an uppercase identifier, enters JSX mode
 * and produces JSX tokens until the matching closing tag.
 */

#include "calynda_jsx_tokens.h"
#include "tokenizer.h"

#include <ctype.h>
#include <string.h>

void jsx_tokenizer_init(JsxTokenizer *jt, const char *source)
{
    tokenizer_init(&jt->base, source);
    jt->in_jsx = false;
    jt->in_jsx_tag = false;
    jt->jsx_depth = 0;
    jt->jsx_expr_depth = 0;
}

/* Helper: make a JsxToken from a base Token */
static JsxToken jsx_from_base(Token t)
{
    JsxToken jt;
    jt.jsx_type = t.type;
    jt.start = t.start;
    jt.length = t.length;
    jt.line = t.line;
    jt.column = t.column;
    jt.tag_name[0] = '\0';
    return jt;
}

/* Helper: make a JsxToken with a specific JSX type */
static JsxToken jsx_make(int jsx_type, const char *start, size_t length,
                          int line, int column)
{
    JsxToken jt;
    jt.jsx_type = jsx_type;
    jt.start = start;
    jt.length = length;
    jt.line = line;
    jt.column = column;
    jt.tag_name[0] = '\0';
    return jt;
}

/*
 * Peek ahead in the source to see if '<' starts a JSX tag.
 * JSX tags start with '<' followed by an uppercase letter (component)
 * or a known lowercase widget name.
 */
static bool looks_like_jsx_open(const char *pos)
{
    if (*pos != '<') return false;
    const char *next = pos + 1;
    /* Skip whitespace */
    while (*next == ' ' || *next == '\t') next++;
    /* JSX: uppercase letter = component, or known element names */
    return isupper((unsigned char)*next) ||
           strncmp(next, "Window", 6) == 0 ||
           strncmp(next, "Button", 6) == 0 ||
           strncmp(next, "Text", 4) == 0 ||
           strncmp(next, "Image", 5) == 0 ||
           strncmp(next, "Show", 4) == 0 ||
           strncmp(next, "For", 3) == 0;
}

/*
 * Scan a JSX tag name (alphanumeric + period for namespaced components).
 */
static size_t scan_tag_name(const char *start, char *out, size_t out_size)
{
    size_t i = 0;
    while (i < out_size - 1 &&
           (isalnum((unsigned char)start[i]) || start[i] == '.' || start[i] == '_')) {
        out[i] = start[i];
        i++;
    }
    out[i] = '\0';
    return i;
}

/*
 * Scan JSX text content (between tags, not inside { } or < >).
 */
static JsxToken scan_jsx_text(JsxTokenizer *jt)
{
    const char *start = jt->base.current;
    int line = jt->base.line;
    int col = jt->base.column;

    while (*jt->base.current != '\0' &&
           *jt->base.current != '<' &&
           *jt->base.current != '{') {
        if (*jt->base.current == '\n') {
            jt->base.line++;
            jt->base.column = 1;
        } else {
            jt->base.column++;
        }
        jt->base.current++;
    }

    size_t len = (size_t)(jt->base.current - start);
    if (len == 0) {
        /* Shouldn't happen, but safety */
        return jsx_make(TOK_ERROR, start, 0, line, col);
    }

    return jsx_make(TOK_JSX_TEXT, start, len, line, col);
}

JsxToken jsx_tokenizer_next(JsxTokenizer *jt)
{
    /* Inside a JSX expression { ... } — delegate to base tokenizer */
    if (jt->in_jsx && jt->jsx_expr_depth > 0) {
        Token t = tokenizer_next(&jt->base);

        if (t.type == TOK_LBRACE) {
            jt->jsx_expr_depth++;
            return jsx_from_base(t);
        }
        if (t.type == TOK_RBRACE) {
            jt->jsx_expr_depth--;
            if (jt->jsx_expr_depth == 0) {
                /* Back to JSX context */
                JsxToken jt_tok = jsx_make(TOK_JSX_EXPR_END, t.start, t.length, t.line, t.column);
                return jt_tok;
            }
            return jsx_from_base(t);
        }
        return jsx_from_base(t);
    }

    /* Inside a JSX opening tag's attributes */
    if (jt->in_jsx && jt->in_jsx_tag) {
        /* Skip whitespace */
        while (*jt->base.current == ' ' || *jt->base.current == '\t' ||
               *jt->base.current == '\n' || *jt->base.current == '\r') {
            if (*jt->base.current == '\n') {
                jt->base.line++;
                jt->base.column = 1;
            } else {
                jt->base.column++;
            }
            jt->base.current++;
        }

        /* Self-close /> */
        if (jt->base.current[0] == '/' && jt->base.current[1] == '>') {
            JsxToken tok = jsx_make(TOK_JSX_SELF_CLOSE, jt->base.current, 2,
                                    jt->base.line, jt->base.column);
            jt->base.current += 2;
            jt->base.column += 2;
            jt->in_jsx_tag = false;
            jt->jsx_depth--;
            if (jt->jsx_depth == 0) jt->in_jsx = false;
            return tok;
        }

        /* Tag end > */
        if (*jt->base.current == '>') {
            JsxToken tok = jsx_make(TOK_JSX_TAG_END, jt->base.current, 1,
                                    jt->base.line, jt->base.column);
            jt->base.current++;
            jt->base.column++;
            jt->in_jsx_tag = false;
            return tok;
        }

        /* Expression attribute value { */
        if (*jt->base.current == '{') {
            JsxToken tok = jsx_make(TOK_JSX_EXPR_START, jt->base.current, 1,
                                    jt->base.line, jt->base.column);
            jt->base.current++;
            jt->base.column++;
            jt->jsx_expr_depth = 1;
            return tok;
        }

        /* Attribute name or = — use base tokenizer */
        Token t = tokenizer_next(&jt->base);
        return jsx_from_base(t);
    }

    /* Inside JSX content (between tags) */
    if (jt->in_jsx && !jt->in_jsx_tag) {
        /* Skip whitespace-only text */
        const char *p = jt->base.current;
        while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') {
            if (*p == '\n') {
                jt->base.line++;
                jt->base.column = 1;
            } else {
                jt->base.column++;
            }
            p++;
        }
        jt->base.current = p;

        if (*jt->base.current == '\0') {
            Token eof = {TOK_EOF, jt->base.current, 0, jt->base.line, jt->base.column};
            return jsx_from_base(eof);
        }

        /* Expression child { */
        if (*jt->base.current == '{') {
            JsxToken tok = jsx_make(TOK_JSX_EXPR_START, jt->base.current, 1,
                                    jt->base.line, jt->base.column);
            jt->base.current++;
            jt->base.column++;
            jt->jsx_expr_depth = 1;
            return tok;
        }

        /* Closing tag </ */
        if (jt->base.current[0] == '<' && jt->base.current[1] == '/') {
            const char *start = jt->base.current;
            int line = jt->base.line;
            int col = jt->base.column;
            jt->base.current += 2;
            jt->base.column += 2;

            JsxToken tok = jsx_make(TOK_JSX_CLOSE_START, start, 2, line, col);
            scan_tag_name(jt->base.current, tok.tag_name, sizeof(tok.tag_name));
            size_t name_len = strlen(tok.tag_name);
            tok.length = 2 + name_len;
            jt->base.current += name_len;
            jt->base.column += (int)name_len;

            /* Skip to > */
            while (*jt->base.current != '>' && *jt->base.current != '\0') {
                jt->base.current++;
                jt->base.column++;
            }
            if (*jt->base.current == '>') {
                jt->base.current++;
                jt->base.column++;
            }

            jt->jsx_depth--;
            if (jt->jsx_depth == 0) jt->in_jsx = false;
            return tok;
        }

        /* Nested opening tag < */
        if (*jt->base.current == '<') {
            const char *start = jt->base.current;
            int line = jt->base.line;
            int col = jt->base.column;
            jt->base.current++;
            jt->base.column++;

            JsxToken tok = jsx_make(TOK_JSX_OPEN_START, start, 1, line, col);
            scan_tag_name(jt->base.current, tok.tag_name, sizeof(tok.tag_name));
            size_t name_len = strlen(tok.tag_name);
            tok.length = 1 + name_len;
            jt->base.current += name_len;
            jt->base.column += (int)name_len;

            jt->in_jsx_tag = true;
            jt->jsx_depth++;
            return tok;
        }

        /* Text content */
        return scan_jsx_text(jt);
    }

    /* Outside JSX — use normal tokenizer, but watch for JSX entry */
    Token t = tokenizer_next(&jt->base);

    /* Check if '<' could be JSX */
    if (t.type == TOK_LT) {
        /* Look at what follows in the source */
        const char *after = jt->base.current;
        while (*after == ' ' || *after == '\t') after++;

        if (isupper((unsigned char)*after)) {
            /* Enter JSX mode — reparse as JSX open tag */
            JsxToken tok = jsx_make(TOK_JSX_OPEN_START, t.start, t.length,
                                    t.line, t.column);
            scan_tag_name(jt->base.current, tok.tag_name, sizeof(tok.tag_name));
            size_t name_len = strlen(tok.tag_name);
            tok.length = 1 + name_len;
            jt->base.current += name_len;
            jt->base.column += (int)name_len;

            jt->in_jsx = true;
            jt->in_jsx_tag = true;
            jt->jsx_depth = 1;
            return tok;
        }
    }

    return jsx_from_base(t);
}
