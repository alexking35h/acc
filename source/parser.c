#include <stdlib.h>

#include <stdio.h>

#include "ast.h"
#include "parser.h"
#include "scanner.h"
#include "token.h"

/*
 * Generate the AST
 *
 * This method implements the recursive descent parsing over the input tokens.
 * This method returns a pointer to the root node in the AST.
 */
AstNode *Parser_generate_ast(Parser *parser) {
  return Parser_expression(parser);
}

/*
 * Initialize the Parser instance.
 */
Parser *Parser_init(Scanner *scanner, Error *error) {
  Parser *parser = calloc(1, sizeof(Parser));
  parser->scanner = scanner;
  parser->error = error;

  return parser;
}

/*
 * Destroy the Parser instance.
 */
void Parser_destroy(Parser *parser) { free(parser); }

/*
 * If the next token matches, advance the token stream, and return the token.
 * Return NULL otherwise.
 */
Token *Parser_match_token(Parser *parser, TokenType token_type) {
  if (parser->next_token == NULL)
    parser->next_token = Scanner_get_next(parser->scanner);

  if (parser->next_token->type == token_type) {
    Token *return_token = parser->next_token;
    parser->next_token = NULL;
    return return_token;
  }
  return NULL;
}

/*
 * Check out the next token.
 */
Token *Parser_peek_token(Parser *parser) {
  if (parser->next_token == NULL)
    parser->next_token = Scanner_get_next(parser->scanner);

  return parser->next_token;
}

/*
 * If the next token does not match, report an error and return false.
 */
bool Parser_consume_token(Parser *parser, TokenType token_type) {
  if (NULL == Parser_match_token(parser, token_type)) {
    return false;
  }
  return true;
}
