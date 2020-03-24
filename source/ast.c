#include "ast.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int Ast_pretty_print_primary(AstNode* ast_node, char* buffer, int buffer_len);
int Ast_pretty_print_postfix(AstNode* ast_node, char* buffer, int buffer_len);
int Ast_pretty_print_binary(AstNode* ast_node, char* buffer, int buffer_len);

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
int Ast_pretty_print(AstNode* ast_node, char* buffer, int buffer_len) {
  switch (ast_node->type) {
    case PRIMARY:
      return Ast_pretty_print_primary(ast_node, buffer, buffer_len);

    case POSTFIX:
      return Ast_pretty_print_postfix(ast_node, buffer, buffer_len);

    case BINARY:
      return Ast_pretty_print_binary(ast_node, buffer, buffer_len);

    case UNARY:
      break;
  }
  return 0;
}

int Ast_pretty_print_binary(AstNode* ast_node, char* buffer, int buffer_len) {
  int l = snprintf(buffer, buffer_len, "(BINARY ");
  l += Ast_pretty_print(ast_node->binary.left, buffer + l, buffer_len - l);
  l += snprintf(buffer + l, buffer_len - l, ", %s, ",
                ast_node->binary.op->lexeme);
  l += Ast_pretty_print(ast_node->binary.right, buffer + l, buffer_len - l);
  return l + snprintf(buffer + l, buffer_len - l, ")");
}

int Ast_pretty_print_primary(AstNode* ast_node, char* buffer, int buffer_len) {
  int l = snprintf(buffer, buffer_len, "(PRIMARY ");

  switch (ast_node->primary.type) {
    case PRIMARY_IDENTIFIER:
      l += snprintf(buffer + l, buffer_len - l, "%s",
                    ast_node->primary.identifier->lexeme);
      break;

    case PRIMARY_CONSTANT:
      l += snprintf(buffer + l, buffer_len - l, "%s",
                    ast_node->primary.constant->lexeme);
      break;

    case PRIMARY_STRING_LITERAL:
      l += snprintf(buffer + l, buffer_len - l, "%s",
                    ast_node->primary.string_literal->lexeme);
      break;
  }
  return l + snprintf(buffer + l, buffer_len - l, ")");
}

int Ast_pretty_print_postfix(AstNode* ast_node, char* buffer, int buffer_len) {
  int l = snprintf(buffer, buffer_len, "(POSTFIX ");
  l += Ast_pretty_print(ast_node->postfix.left, buffer + l, buffer_len - 1);
  l += snprintf(buffer + l, buffer_len - l, ", ");

  switch (ast_node->postfix.type) {
    case POSTFIX_ARRAY_INDEX:
      l += Ast_pretty_print(ast_node->postfix.index_expression, buffer + l,
                            buffer_len - l);

      break;

    case POSTFIX_OP:
      break;
  }
  return l + snprintf(buffer + l, buffer_len - 1, ")");
}
