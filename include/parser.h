#ifndef __PARSER__
#define __PARSER__

#include "ast.h"
#include "scanner.h"
#include "token.h"

typedef struct Parser_t {
  // Scanner and error instances.
  Scanner *scanner;
  Error *error;

  // Next unread token.
  Token *next_token;

} Parser;

/*
 * Initialize the Parser instance.
 */
Parser *Parser_init(Scanner *, Error *);

/*
 * Destroy the Parser instance.
 */
void Parser_destroy(Parser *);

/*
 * Generate the AST
 *
 * This method implements the recursive descent parsing over the input tokens.
 * This method returns a pointer to the root node in the AST.
 */
AstNode *Parser_generate_ast(Parser *);

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
 * If the next token does not match, report an error and return false.
 */
bool Parser_consume_token(Parser *, TokenType);

/*
 * Create a new token, for the purposes of desugauring syntax.
 *
 * This function allocates a new token (using the Scanner_create_token
 * function), and initializes it.
 */
Token *Parser_create_fake_token(Parser *parser, TokenType type, char *lexeme);

/*
 * Recursive descent parser function definitions.
 */
AstNode *Parser_expression(Parser *parser);
AstNode *Parser_statement(Parser *parser);
AstNode *Parser_declaration(Parser *parser);

#endif
