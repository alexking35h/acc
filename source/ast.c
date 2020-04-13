#include <stdlib.h>
#include <string.h>

#include "ast.h"

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
StmtAstNode* Ast_create_stmt_node(StmtAstNode ast_node) {
  StmtAstNode* node = calloc(1, sizeof(StmtAstNode));

  memcpy(node, &ast_node, sizeof(ast_node));
  return node;
}
