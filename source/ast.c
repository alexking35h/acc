#include "ast.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

AstNode* Ast_create_node(AstNode ast_node) {
  AstNode* node = calloc(1, sizeof(AstNode));

  memcpy(node, &ast_node, sizeof(ast_node));
  return node;
}

int Ast_pretty_print(AstNode* ast_node, char* buffer, int len) {
  int l;

  switch (ast_node->type) {
    case UNARY:
      l = snprintf(buffer, len,"(UNARY ");
      l += snprintf(buffer+l, len-l, "%s, ", ast_node->node.unary.op->lexeme);
      l += Ast_pretty_print(ast_node->node.unary.right, buffer+l, len-l);
      return l + snprintf(buffer+l, len-l, ")");
  
    case BINARY:
      l = snprintf(buffer, len, "(BINARY ");
      l += Ast_pretty_print(ast_node->node.binary.left, buffer + l, len - l);
      l += snprintf(buffer + l, len - l, ", ");
      l += Ast_pretty_print(ast_node->node.binary.right, buffer + l, len - l);
      return l + snprintf(buffer + l, len - l, ")");

    case PRIMARY:
      l = snprintf(buffer, len, "(PRIMARY ");
      switch (ast_node->node.primary.type) {
        case PRIMARY_STRING_LITERAL:
          l += snprintf(buffer + l, len - l, "%s",
                        ast_node->node.primary.string_literal->lexeme);
          break;
        case PRIMARY_CONSTANT:
          l += snprintf(buffer + l, len - 1, "%s",
                        ast_node->node.primary.constant->lexeme);
          break;
        case PRIMARY_IDENTIFIER:
          l += snprintf(buffer + l, len - 1, "$%s",
                        ast_node->node.primary.identifier->lexeme);
          break;
      }
      return l + snprintf(buffer + l, len - l, ")");

    case POSTFIX:
      l = snprintf(buffer, len, "(POSTFIX ");
      l += Ast_pretty_print(ast_node->node.postfix.left, buffer + l, len - l);
      switch (ast_node->node.postfix.type) {
        case POSTFIX_ARRAY_INDEX:
          l += snprintf(buffer + l, len - l, ", ");
          l += Ast_pretty_print(ast_node->node.postfix.index_expression,
                                buffer + l, len - l);
          break;
        case POSTFIX_OP:
          l += snprintf(buffer + l, len - 1, ", %s",
                        ast_node->node.postfix.op->lexeme);
          break;
      }
      return l + snprintf(buffer + l, len - l, ")");
  }
  return 0;
}
