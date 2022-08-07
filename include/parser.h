/*
 * Recursive-descent Parser
 *
 * The parser generates the Abstract Syntax Tree from the sequence
 * of tokens provided by the scanner. The parser is split into three
 * parts:
 *  - Expressions (parser_expression.c)
 *  - Declarations (parser_declaration.c)
 *  - Statements (parser_statement.c)
 *
 * Therefore, this header file declares functions used outside the parser:
 *  - Parser_init()
 *  - Parser_destroy()
 *  - Parser_translation_unit()
 *
 * This header file also declares functions used internally within the parser.
 * Common code is defined in parser.c
 *  - Parser_match_token()
 *  - Parser_peek_token()
 *  - Parser_peek_next_token()
 *  - Parser_consume_token()
 *  - Parser_advance_token()
 *
 * The parser implements panic-mode error recovery - if an error occurs during parsing,
 * the parser synchronises on the next semicolon in the token stream.
 *
 * The entry point for the parser is Parser_translation_unit(), which returns the
 * AST root node.
 */

#ifndef __PARSER__
#define __PARSER__

#include <setjmp.h>

#include "ast.h"
#include "ctype.h"
#include "scanner.h"
#include "token.h"

typedef struct Parser_t Parser;

/*
 * Initialize the Parser instance.
 */
Parser *Parser_init(Scanner *, ErrorReporter *);

/*
 * Destroy the Parser instance.
 */
void Parser_destroy(Parser *);

/*
 * Recursive descent parser function definitions.
 */
ExprAstNode *Parser_expression(Parser *parser);
DeclAstNode *Parser_declaration(Parser *parser);
DeclAstNode *Parser_translation_unit(Parser *parser);
StmtAstNode *Parser_compound_statement(Parser *parser);

/*
 * Check if the parser reached the end of the input
 */
bool Parser_at_end(Parser * parser);

#endif
