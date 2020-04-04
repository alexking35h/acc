/*
 * AST node definitions.
 *
 * AST nodes types are separated into two categories:
 * - expression - ExprAstNode
 * - declaration - DeclAstNode
 *
 * There are no places in the parser where DeclAstNode and ExprAstNode
 * are interchangeable. Having separate struct types will give
 * a compiler error if a mistake is made. Also, reduces the number
 * of redundant fields.
 */

#ifndef __AST__
#define __AST__

#include "ctype.h"
#include "token.h"

// Macro definitions for creating AST nodes.
// These should be used instead of calling Ast_create_node directly.

#define EXPR_BINARY(...) \
  Ast_create_expr_node((ExprAstNode){.type = BINARY, .binary = {__VA_ARGS__}})
#define EXPR_UNARY(...) \
  Ast_create_expr_node((ExprAstNode){.type = UNARY, .unary = {__VA_ARGS__}})
#define EXPR_PRIMARY(...) \
  Ast_create_expr_node((ExprAstNode){.type = PRIMARY, .primary = {__VA_ARGS__}})
#define EXPR_POSTFIX(...) \
  Ast_create_expr_node((ExprAstNode){.type = POSTFIX, .postfix = {__VA_ARGS__}})
#define EXPR_CAST(...) \
  Ast_create_expr_node((ExprAstNode){.type = CAST, .cast = {__VA_ARGS__}})
#define EXPR_TERTIARY(...) \
  Ast_create_expr_node(    \
      (ExprAstNode){.type = TERTIARY, .tertiary = {__VA_ARGS__}})
#define EXPR_ASSIGN(...) \
  Ast_create_expr_node((ExprAstNode){.type = ASSIGN, .assign = {__VA_ARGS__}})

#define DECL(...) Ast_create_decl_node((DeclAstNode){__VA_ARGS__})

typedef struct ExprAstNode_t {
  // Type of this AST node.
  enum {
    BINARY,
    UNARY,
    PRIMARY,
    POSTFIX,
    CAST,
    TERTIARY,
    ASSIGN,
  } type;

  // Anonymous union for each node sub-type.
  union {
    // Binary
    struct {
      struct ExprAstNode_t* left;
      Token* op;
      struct ExprAstNode_t* right;
    } binary;

    // Unary
    struct {
      Token* op;
      struct ExprAstNode_t* right;
    } unary;

    // Primary
    struct {
      Token* identifier;
      Token* constant;
      Token* string_literal;
    } primary;

    // Postfix
    struct {
      Token* op;
      struct ExprAstNode_t* index_expression;
      struct ExprAstNode_t* left;
    } postfix;

    // Cast
    struct {
      Token* type_token;
      struct ExprAstNode_t* right;
    } cast;

    // Tertiary
    struct {
      struct ExprAstNode_t* condition_expr;
      struct ExprAstNode_t* expr_true;
      struct ExprAstNode_t* expr_false;
    } tertiary;

    // Assign
    struct {
      struct ExprAstNode_t* left;
      struct ExprAstNode_t* right;
    } assign;
  };

} ExprAstNode;

typedef struct DeclAstNode {
  Token* identifier;
  CType* type;
  struct DeclAstNode* next;

} DeclAstNode;

/*
 * Create a new AST node
 *
 * This function takes a ExprAstNode by value (presumably allocated
 * automatically), and copies it to a new AstNode allocated dynamically.
 *
 * E.g.:
 * > Ast_create_new((AstNode){.type=...});
 */
ExprAstNode* Ast_create_expr_node(ExprAstNode ast_node);

/*
 * Create a new Decl AST node
 *
 * This function takes a DeclAstNode by value and copies it to a new AstNode
 * allocated dynamically.
 */
DeclAstNode* Ast_create_decl_node(DeclAstNode ast_node);

/*
 * Generate a string for the given AST node.
 *
 * This function generates a string representation for a given AST node.
 */
int Ast_pretty_print(ExprAstNode* ast_node, char* buffer, int buffer_len);

#endif
