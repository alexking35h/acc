#ifndef __PARSER__
#define __PARSER__

#include <setjmp.h>

#include "ast.h"
#include "scanner.h"
#include "token.h"

typedef struct Parser_t {
  // Scanner and error instances.
  Scanner *scanner;
  Error *error;

  // Next unread token.
  Token *next_token;

  jmp_buf panic_jmp;

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
void Parser_consume_token(Parser *, TokenType);

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

#define CATCH_ERROR(parser) (setjmp(parser->panic_jmp) != 0)
#define THROW_ERROR(parser) longjmp(parser->panic_jmp, 1)

void Parser_synchronise_token(Parser *, TokenType[]);


/*
 * Recursive descent parser function definitions.
 */
ExprAstNode *Parser_expression(Parser *parser);
ExprAstNode *Parser_assignment_expression(Parser *parser);

DeclAstNode* Parser_declaration(Parser *parser);
DeclAstNode *Parser_translation_unit(Parser *parser);

StmtAstNode *Parser_compound_statement(Parser *parser);

CType *Parser_type_name(Parser *parser);

#endif
