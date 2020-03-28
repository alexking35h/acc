#include "ast.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int primary(AstNode* node, char* buf, int len);
static int postfix(AstNode* node, char* buf, int len);
static int binary(AstNode* node, char* buf, int len);
static int unary(AstNode* node, char* buf, int len);
static int tertiary(AstNode* node, char* buf, int len);
static int assign(AstNode* node, char* buf, int len);
static int expr(AstNode* node, char* buf, int len);

/*
 * Create a new AST node
 *
 * This function takes a AstNode by value (presumably allocated automatically),
 * and copies it to a new AstNode allocated dynamically.
 *
 * E.g.:
 * > Ast_create_new((AstNode){.type=...});
 */
AstNode* Ast_create_node(AstNode ast_node) {
  AstNode* node = calloc(1, sizeof(AstNode));

  memcpy(node, &ast_node, sizeof(ast_node));
  return node;
}

/*
 * Generate a string for the given AST node.
 *
 * This function generates a string representation for a given AST node.
 */
int Ast_pretty_print(AstNode* node, char* buf, int len) {
  switch (node->type) {
    case PRIMARY:
      return primary(node, buf, len);

    case POSTFIX:
      return postfix(node, buf, len);

    case BINARY:
      return binary(node, buf, len);

    case UNARY:
      return unary(node, buf, len);

    case TERTIARY:
      return tertiary(node, buf, len);

    case CAST:
      return 0;

    case ASSIGN:
      return assign(node, buf, len);

    case EXPR:
      return expr(node, buf, len);
  }
  return 0;
}

static int primary(AstNode* node, char* buf, int len) {
  int l = snprintf(buf, len, "(P ");

  switch (node->primary.type) {
    case PRIMARY_IDENTIFIER:
      l += snprintf(buf + l, len - l, "%s",
                    node->primary.identifier->lexeme);
      break;

    case PRIMARY_CONSTANT:
      l += snprintf(buf + l, len - l, "%s", node->primary.constant->lexeme);
      break;

    case PRIMARY_STRING_LITERAL:
      l += snprintf(buf + l, len - l, "%s",
                    node->primary.string_literal->lexeme);
      break;
  }
  return l + snprintf(buf + l, len - l, ")");
}

static int postfix(AstNode* node, char* buf, int len) {
  int l = snprintf(buf, len, "(PF ");
  l += Ast_pretty_print(node->postfix.left, buf + l, len - 1);
  l += snprintf(buf + l, len - l, ", ");

  switch (node->postfix.type) {
    case POSTFIX_ARRAY_INDEX:
      l += Ast_pretty_print(node->postfix.index_expression, buf + l,
                            len - l);

      break;

    case POSTFIX_OP:
      break;
  }
  return l + snprintf(buf + l, len - 1, ")");
}

static int binary(AstNode* node, char* buf, int len) {
  int l = snprintf(buf, len, "(B ");
  l += Ast_pretty_print(node->binary.left, buf + l, len - l);
  l += snprintf(buf + l, len - l, ", %s, ", node->binary.op->lexeme);
  l += Ast_pretty_print(node->binary.right, buf + l, len - l);
  return l + snprintf(buf + l, len - l, ")");
}

static int unary(AstNode* node, char* buf, int len) {
  int l = snprintf(buf, len, "(U ");
  l += snprintf(buf + l, len - l, "%s, ", node->unary.op->lexeme);
  l += Ast_pretty_print(node->unary.right, buf + l, len - 1);
  return l + snprintf(buf + l, len - l, ")");
}

static int tertiary(AstNode* node, char* buf, int len) {
  int l = snprintf(buf, len, "(T ");
  l += Ast_pretty_print(node->tertiary.condition_expr, buf + l, len - l);
  l += snprintf(buf + l, len - l, ", ");
  l += Ast_pretty_print(node->tertiary.expr_true, buf + l, len - l);
  l += snprintf(buf + l, len - l, ", ");
  l += Ast_pretty_print(node->tertiary.expr_false, buf + l, len - l);
  return l + snprintf(buf + l, len - l, ")");
}

static int assign(AstNode* node, char* buf, int len) {
  int l = snprintf(buf, len, "(A ");
  l += Ast_pretty_print(node->assign.left, buf + l, len - l);
  l += snprintf(buf + l, len - l, ", ");
  l += Ast_pretty_print(node->assign.right, buf + l, len - l);
  return l + snprintf(buf + l, len - l, ")");
}

static int expr(AstNode* node, char* buf, int len) {
  int l = snprintf(buf, len, "(E ");
  l += Ast_pretty_print(node->expr.expr, buf + l, len - l);
  l += snprintf(buf + l, len - l, ", ");
  l += Ast_pretty_print(node->expr.next, buf + l, len - l);
  return l + snprintf(buf + l, len - l, ")");
}
