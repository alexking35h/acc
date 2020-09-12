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

typedef struct Parser_t
{
    // Scanner and error instances.
    Scanner *scanner;
    ErrorReporter *error_reporter;

    // Next unread token.
    Token *next_token[2];
    int next_token_index;

    jmp_buf panic_jmp;

} Parser;

/*
 * Initialize the Parser instance.
 */
Parser *Parser_init(Scanner *, ErrorReporter *);

/*
 * Destroy the Parser instance.
 */
void Parser_destroy(Parser *);

/*
 * If the next token matches, advance the token stream, and return the token.
 * Return NULL otherwise.
 */
Token *Parser_match_token(Parser *, TokenType *);

/*
 * Check out the next token.
 */
Token *Parser_peek_token(Parser *);

/*
 * Check out the next+1 token.
 */
Token *Parser_peek_next_token(Parser *);

/*
 * If the next token does not match, report an error and return false.
 */
Token *Parser_consume_token(Parser *, TokenType);

/*
 * Advance the parser to the next token
 */
void Parser_advance_token(Parser *);

/*
 * Create a new token, for the purposes of desugauring syntax.
 *
 * This function allocates a new token (using the Scanner_create_token
 * function), and initializes it.
 */
Token *Parser_create_fake_token(Parser *parser, TokenType type, char *lexeme);

/*
 * Rudimentary try/catch statements using setjmp and longjmp. E.g.:
 *  if(CATCH_ERROR(parser)) {
 *    // Error occurred!
 *  } else {
 *    //...
 *    THROW_ERROR(parser)
 *  }
 */
#define CATCH_ERROR(parser) (setjmp(parser->panic_jmp) != 0)
#define THROW_ERROR(parser) longjmp(parser->panic_jmp, 1)

/*
 * Synchronise the token stream input on any token in set types.
 * The array types must end with NAT. (E.g., {SEMICOLON, EOF, NAT})
 */
void Parser_sync_token(Parser *parser, TokenType types[]);

/*
 * Recursive descent parser function definitions.
 */
ExprAstNode *Parser_expression(Parser *parser);
DeclAstNode *Parser_declaration(Parser *parser);
DeclAstNode *Parser_translation_unit(Parser *parser);
StmtAstNode *Parser_compound_statement(Parser *parser);
CType *Parser_type_name(Parser *parser);

#endif
