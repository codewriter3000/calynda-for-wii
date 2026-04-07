#ifndef CALYNDA_JSX_TOKENS_H
#define CALYNDA_JSX_TOKENS_H

/*
 * calynda_jsx_tokens.h — Additional token types for JSX support.
 *
 * These extend the base TokenType enum. Since C enums can't be
 * extended, we define JSX tokens as offsets from TOK_COUNT.
 *
 * The JSX-aware tokenizer layer wraps the standard tokenizer and
 * translates '<' / '>' / '/' / '=' into JSX-specific tokens when
 * in JSX context.
 */

#include "tokenizer.h"

/* JSX token types, starting after TOK_COUNT */
typedef enum {
    TOK_JSX_OPEN_START = TOK_COUNT,  /* <Identifier   (start of opening tag)    */
    TOK_JSX_CLOSE_START,              /* </Identifier  (start of closing tag)    */
    TOK_JSX_TAG_END,                  /* >             (end of opening tag)      */
    TOK_JSX_SELF_CLOSE,               /* />            (self-closing tag end)    */
    TOK_JSX_TEXT,                      /* plain text between tags                */
    TOK_JSX_EXPR_START,               /* {             (expression in JSX child) */
    TOK_JSX_EXPR_END,                 /* }             (end of JSX expression)   */
    TOK_JSX_STYLE_OPEN,               /* style={       (style attribute start)   */
    TOK_JSX_COUNT
} JsxTokenType;

/*
 * JSX token — wraps a standard Token with additional JSX metadata.
 */
typedef struct {
    int          jsx_type;   /* JsxTokenType or TokenType */
    const char  *start;
    size_t       length;
    int          line;
    int          column;
    /* For TOK_JSX_OPEN_START / TOK_JSX_CLOSE_START: the tag name */
    char         tag_name[64];
} JsxToken;

/*
 * JSX tokenizer state — wraps the standard tokenizer.
 */
typedef struct {
    Tokenizer   base;
    bool        in_jsx;           /* currently inside JSX context     */
    bool        in_jsx_tag;       /* inside an opening tag's attrs    */
    int         jsx_depth;        /* nesting depth of JSX elements    */
    int         jsx_expr_depth;   /* brace depth inside JSX { expr }  */
} JsxTokenizer;

void jsx_tokenizer_init(JsxTokenizer *jt, const char *source);

/*
 * Get next token. Returns standard tokens outside JSX context,
 * JSX tokens when inside JSX.
 */
JsxToken jsx_tokenizer_next(JsxTokenizer *jt);

/*
 * Check if a token type is a JSX token.
 */
static inline bool is_jsx_token(int type) {
    return type >= TOK_COUNT && type < TOK_JSX_COUNT;
}

#endif /* CALYNDA_JSX_TOKENS_H */
