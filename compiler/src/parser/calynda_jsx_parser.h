#ifndef CALYNDA_JSX_PARSER_H
#define CALYNDA_JSX_PARSER_H

/*
 * calynda_jsx_parser.h — JSX parser for Calynda.
 *
 * Extends the standard parser to handle JSX expressions inside
 * Calynda source code. A JSX expression like <Window>...</Window>
 * is parsed as an AstExpression of kind AST_EXPR_JSX.
 *
 * Usage: called from parse_primary_expression() when a '<' token
 * is followed by an uppercase identifier.
 */

#include "parser.h"
#include "calynda_jsx_ast.h"
#include "calynda_jsx_tokens.h"

/*
 * Parse a JSX element starting from the current token position.
 * Expects the parser to be positioned at the '<' of '<TagName'.
 *
 * Returns a JsxElement* on success, NULL on parse error.
 *
 * The parser's token stream must have been produced by JsxTokenizer
 * (or the tokens array must contain JSX token types).
 */
JsxElement *jsx_parse_element(JsxTokenizer *jt);

/*
 * Parse JSX attributes inside an opening tag.
 * Called after the tag name, before '>' or '/>'.
 */
bool jsx_parse_attributes(JsxTokenizer *jt, JsxElement *element);

/*
 * Parse JSX children between opening and closing tags.
 */
bool jsx_parse_children(JsxTokenizer *jt, JsxElement *element);

/*
 * Parse a style attribute value: style={{ width: 100, color: #FF0000 }}
 */
bool jsx_parse_style_object(JsxTokenizer *jt, JsxAttribute *attr);

#endif /* CALYNDA_JSX_PARSER_H */
