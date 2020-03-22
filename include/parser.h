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
 * Generate the AST
 *
 * This method implements the recursive descent parsing over the input tokens.
 * This method returns a pointer to the root node in the AST.
 */
AstNode *Parser_generate_ast(Parser *);

/*
 * Initialize the Parser instance.
 */
Parser *Parser_init(Scanner *, Error *);

/*
 * Destroy the Parser instance.
 */
void Parser_destroy(Parser *);

/*
 * If the next token matches, advance the token stream, and return the token.
 * Return NULL otherwise.
 */
Token *Parser_match_token(Parser *, TokenType);

/*
 * Check out the next token.
 */
Token *Parser_peek_token(Parser *);

/*
 * If the next token does not match, report an error and return false.
 */
bool Parser_consume_token(Parser *, TokenType);

/* Recursive-descent parsing functions */
AstNode *Parser_expression(Parser *);
AstNode *Parser_assignment_expression(Parser *);
AstNode *Parser_conditional_expression(Parser *);
AstNode *Parser_logical_or_expression(Parser *);
AstNode *Parser_logical_and_expression(Parser *);
AstNode *Parser_unary_expression(Parser *);
AstNode *Parser_postfix_expression(Parser *);
AstNode *Parser_primary_expression(Parser *);

#endif
