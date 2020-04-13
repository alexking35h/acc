/*
 * AST node definitions.
 *
 * AST nodes types are separated into two categories:
 * - expression - ExprAstNode
 * - declaration - DeclAstNode
 * - statement - StmtAstNode
 *
 * There are no places in the parser where these are interchangeable.
 * Having separate struct types will give a compiler error if a mistake is
 * made.
 */

#ifndef __AST__
#define __AST__

#include "ctype.h"
#include "token.h"

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
      CType* type;
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
  // Declarators are typically 'concrete', meaning tied to an identifier
  // (e.g. `int a;`). However we can also have 'abstract declarators'
  // in cast expressions and parameter lists (e.g., `void (int*, char, void*)`.
  enum { CONCRETE, ABSTRACT } decl_type;

  // C data type for this declaration.
  CType* type;

  // Identifier token for this declaration.
  Token* identifier;

  // Initial value expression
  ExprAstNode* initializer;

  // Declarations can be concatenated with ',', such as `int a=1, *b;`. The C11
  // grammar in this case is left-recursive; *next points to the nested
  // declaration.
  struct DeclAstNode* next;
} DeclAstNode;

typedef struct StmtAstNode {
  // Type of this statement node.
  enum { DECL, EXPR, BLOCK } type;

  union {
    // Declaration
    struct {
      DeclAstNode* decl;
    } decl;

    // Expression
    struct {
      ExprAstNode* expr;
    } expr;

    // Block statement
    struct {
      struct StmtAstNode* head;
    } block;
  };

  // s
  struct StmtAstNode* next;

} StmtAstNode;

/*
 * Create new AST nodes
 *
 * These functions take an AST by value (presumably allocated automatically),
 * and copy it to a new ExprAstNode/DeclAstNode allocated dynamically.
 *
 * E.g.:
 * > Ast_create_expr_node((ExprAstNode){.primary.identifier=...});
 */
ExprAstNode* Ast_create_expr_node(ExprAstNode ast_node);
DeclAstNode* Ast_create_decl_node(DeclAstNode ast_node);
StmtAstNode* Ast_create_stmt_node(StmtAstNode ast_node);

#endif
