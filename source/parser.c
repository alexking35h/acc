#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ast.h"
#include "parser.h"
#include "scanner.h"
#include "token.h"

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
 * If the next token matches any of the tokens 'token_types', advance the
 * token stream, and return the token. 'token_types' is a NULL terminated
 * array of TokenType.
 */
Token *Parser_match_token(Parser *parser, TokenType *token_types) {
  if (parser->next_token == NULL)
    parser->next_token = Scanner_get_next(parser->scanner);

  for (; *token_types != NAT; token_types++) {
    if (parser->next_token->type != *token_types) continue;

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
  if (NULL == Parser_match_token(parser, (TokenType[]){token_type, NAT})) {
    return false;
  }
  return true;
}

/*
 * Create a new token, for the purposes of desugauring syntax.
 *
 * This function allocates a new token (using the Scanner_create_token
 * function), and initializes it.
 */
Token *Parser_create_fake_token(Parser *parser, TokenType type, char *lexeme) {
  Token *token = Scanner_create_token(parser->scanner);

  token->type = type;
  token->line_number = -1;
  token->lexeme = calloc(1, strlen(lexeme) + 1);
  strcpy(token->lexeme, lexeme);

  return token;
}
