/* Recursive Descent Parser implementation (statement)
 *
 * The recursive descent parser is implemented as a set of mutually recursive
 * functions for each rule in the grammar. This file provides functions for
 * rules in the 'statement' set.
 *
 * Generated by generate_recursive_parser.py on 2020-03-24.
 */

#include <stddef.h>

#include "ast.h"
#include "parser.h"
#include "token.h"

#define STMT_EXPR(...) \
  Ast_create_stmt_node((StmtAstNode){.type = EXPR, .expr = {__VA_ARGS__}})
#define STMT_DECL(...) \
  Ast_create_stmt_node((StmtAstNode){.type = DECL, .decl = {__VA_ARGS__}})
#define STMT_BLOCK(...) \
  Ast_create_stmt_node((StmtAstNode){.type = BLOCK, .block = {__VA_ARGS__}})
#define STMT_WHILE(...) \
  Ast_create_stmt_node( \
      (StmtAstNode){.type = WHILE_LOOP, .while_loop = {__VA_ARGS__}})

#define consume(t) Parser_consume_token(parser, t)
#define peek(t) Parser_peek_token(parser)
#define match(...) Parser_match_token(parser, (TokenType[]){__VA_ARGS__, NAT})
#define advance(...) Parser_advance_token(parser)
#define sync(...) Parser_sync_token(parser, (TokenType[]){__VA_ARGS__, NAT})

#define static

static StmtAstNode* expression_statement(Parser* parser);
static StmtAstNode* iteration_statement(Parser* parser);

_Bool is_decl(TokenType tok) {
  switch (tok) {
    case VOID:
    case CHAR:
    case SHORT:
    case INT:
    case LONG:
    case FLOAT:
    case DOUBLE:
    case SIGNED:
    case UNSIGNED:
      return true;
    default:
      return false;
  }
}

static StmtAstNode* statement(Parser* parser) {  // @TODO
  /*
   * labeled_statement
   * compound_statement
   * expression_statement
   * selection_statement
   * iteration_statement
   * jump_statement
   */
  if (peek()->type == LEFT_BRACE) {
    // Compound statement.
    return Parser_compound_statement(parser);
  }
  if (peek()->type == WHILE) {
    // While statement.
    return iteration_statement(parser);
  }
  return expression_statement(parser);
}
static ExprAstNode* labeled_statement(Parser* parser) {  // @TODO
  /*
   * IDENTIFIER ':' statement
   * CASE constant_expression ':' statement
   * DEFAULT ':' statement
   */

  return NULL;
}
StmtAstNode* Parser_compound_statement(Parser* parser) {  // @TODO
  /*
   * '{' '}'
   * '{'  block_item_list '}'
   */
  StmtAstNode *stmt, *head = NULL, **curr = &stmt;

  consume(LEFT_BRACE);
  while (!match(RIGHT_BRACE)) {
    if (CATCH_ERROR(parser)) {
      // Error occurred parsing the following declaration.
      // synchronise on the next semi-colon, or until we reach the RIGHT brace.
      sync(SEMICOLON, RIGHT_BRACE, END_OF_FILE);

      if (peek()->type == SEMICOLON) advance();
      if (peek()->type == END_OF_FILE) return NULL;
      continue;
    }

    if (is_decl(peek()->type)) {
      // Declaration.
      *curr = STMT_DECL(.decl = Parser_declaration(parser));
    } else {
      // Statement.
      *curr = statement(parser);
    }
    if (!head) head = *curr;
    curr = &((*curr)->next);
  }
  return STMT_BLOCK(.head = head);
}
ExprAstNode* Parser_block_item_list(Parser* parser) {  // @TODO
  /*
   * block_item
   * block_item_list block_item
   */

  return NULL;
}
ExprAstNode* Parser_block_item(Parser* parser) {  // @TODO
  /*
   * declaration
   * statement
   */

  return NULL;
}
static StmtAstNode* expression_statement(Parser* parser) {
  /*
   * expression ';'
   */
  StmtAstNode* stmt = STMT_EXPR(.expr = Parser_expression(parser));
  consume(SEMICOLON);
  return stmt;
}
ExprAstNode* Parser_selection_statement(Parser* parser) {  // @TODO
  /*
   * IF '(' expression ')' statement ELSE statement
   * IF '(' expression ')' statement
   * SWITCH '(' expression ')' statement
   */

  return NULL;
}
StmtAstNode* iteration_statement(Parser* parser) {  // @TODO
  /*
   * WHILE '(' expression ')' statement
   * DO statement WHILE '(' expression ')' '
   */
  if (match(WHILE)) {
    consume(LEFT_PAREN);
    ExprAstNode* expr = Parser_expression(parser);
    consume(RIGHT_PAREN);
    StmtAstNode* block = statement(parser);

    return STMT_WHILE(.expr = expr, .block = block);
  }

  return NULL;
}
ExprAstNode* Parser_jump_statement(Parser* parser) {  // @TODO
  /*
   * GOTO IDENTIFIER '
   */

  return NULL;
}
