#include "ast.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define pp_printf(buf, ...) \
  buf->start += snprintf(buf->start, buf->end - buf->start, __VA_ARGS__);\

typedef struct StringBuffer_t {
  char* start;
  char* end;
} StringBuffer;

static void pp_expr(ExprAstNode* node, StringBuffer* buf);
static void pp_primary(ExprAstNode* node, StringBuffer* buf);
static void pp_postfix(ExprAstNode* node, StringBuffer* buf);
static void pp_binary(ExprAstNode* node, StringBuffer* buf);
static void pp_unary(ExprAstNode* node, StringBuffer* buf);
static void pp_tertiary(ExprAstNode* node, StringBuffer* buf);
static void pp_assign(ExprAstNode* node, StringBuffer* buf);

/* Create new AST nodes
 *
 * These functions take an AST by value (presumably allocated automatically),
 * and copy it to a new ExprAstNode/DeclAstNode allocated dynamically.
 *
 * E.g.:
 * > Ast_create_expr_node((ExprAstNode){.primary.identifier=...});
 */
ExprAstNode* Ast_create_expr_node(ExprAstNode ast_node) {
  ExprAstNode* node = calloc(1, sizeof(ExprAstNode));

  memcpy(node, &ast_node, sizeof(ast_node));
  return node;
}
DeclAstNode* Ast_create_decl_node(DeclAstNode ast_node) {
  DeclAstNode* node = calloc(1, sizeof(DeclAstNode));

  memcpy(node, &ast_node, sizeof(ast_node));
  return node;
}

/*
 * Generate a string for the given ExprAstNode.
 */
int Ast_pretty_print_expr(ExprAstNode* node, char* buf, int len) {
  StringBuffer str_buf = {buf, buf + len};
  pp_expr(node, &str_buf);
  return str_buf.end - str_buf.start;
}

/*
 * Generate a string for the given AST node.
 *
 * This function generates a string representation for a given AST node.
 */
static void pp_expr(ExprAstNode* node, StringBuffer* buf) {
  pp_printf(buf, "(");
  switch (node->type) {
    case PRIMARY:
      pp_primary(node, buf);
      break;
  
    case POSTFIX:
      pp_postfix(node, buf);
      break;
  
    case BINARY:
      pp_binary(node, buf);
      break;
  
    case UNARY:
      pp_unary(node, buf);
      break;
  
    case TERTIARY:
      pp_tertiary(node, buf);
      break;
  
    case CAST:
  
    case ASSIGN:
      pp_assign(node, buf);
      break;
  }
  pp_printf(buf, ")");
}

static void pp_primary(ExprAstNode* node, StringBuffer* buf) {
  pp_printf(buf, "P ");

  if (node->primary.identifier)
  {
    pp_printf(buf, "%s", node->primary.identifier->lexeme);
  }
  else if (node->primary.constant) {
    pp_printf(buf, "%s", node->primary.constant->lexeme);
  }
  else if (node->primary.string_literal) {
    pp_printf(buf, "%s", node->primary.string_literal->lexeme);
  }
}

static void pp_postfix(ExprAstNode* node, StringBuffer* buf) {
  pp_printf(buf, "PF ");
  pp_expr(node->postfix.left, buf);
  pp_printf(buf, ", ");

  if (node->postfix.index_expression)
    pp_expr(node->postfix.index_expression, buf);
}

static void pp_binary(ExprAstNode* node, StringBuffer* buf) {
  pp_printf(buf, "B ");
  pp_expr(node->binary.left, buf);
  pp_printf(buf, ", %s, ", node->binary.op->lexeme);
  pp_expr(node->binary.right, buf);
}

static void pp_unary(ExprAstNode* node, StringBuffer* buf) {
  pp_printf(buf, "U %s, ", node->unary.op->lexeme);
  pp_expr(node->unary.right, buf);
}

static void pp_tertiary(ExprAstNode* node, StringBuffer* buf) {
  pp_printf(buf, "T ");
  pp_expr(node->tertiary.condition_expr, buf);
  pp_printf(buf, ", ");
  pp_expr(node->tertiary.expr_true, buf);
  pp_printf(buf, ", ");
  pp_expr(node->tertiary.expr_false, buf);
}

static void pp_assign(ExprAstNode* node, StringBuffer* buf) {
  pp_printf(buf, "A ");
  pp_expr(node->assign.left, buf);
  pp_printf(buf, ", ");
  pp_expr(node->assign.right, buf);
}
