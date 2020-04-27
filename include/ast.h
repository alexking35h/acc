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
#include "symbol.h"

struct ExprAstNode_t;
struct DeclAstNode_t;
struct StmtAstNode_t;

typedef struct ArgumentListItem_t {
  struct ExprAstNode_t * argument;
  struct ArgumentListItem_t * next;
} ArgumentListItem;

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
      Symbol* symbol;
    } primary;

    // Postfix
    struct {
      Token* op;
      struct ExprAstNode_t* index_expression;
      struct ExprAstNode_t* left;
      struct ArgumentListItem_t* args;
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

typedef struct DeclAstNode_t {
  // Declarators are typically 'concrete', meaning tied to an identifier
  // (e.g. `int a;`). However we can also have 'abstract declarators'
  // in cast expressions and parameter lists (e.g., `void (int*, char, void*)`.
  enum { CONCRETE, ABSTRACT } decl_type;

  // C data type for this declaration.
  CType* type;

  // Identifier token for this declaration.
  Token* identifier;

  // Initial value expression
  union {
    struct ExprAstNode_t* initializer;
    struct StmtAstNode_t* body;
  };

  Symbol* symbol;

  // Declarations can be concatenated with ',', such as `int a=1, *b;`. The C11
  // grammar in this case is left-recursive; *next points to the nested
  // declaration.
  struct DeclAstNode_t* next;
} DeclAstNode;

typedef struct StmtAstNode_t {
  // Type of this statement node.
  enum { DECL, EXPR, BLOCK, WHILE_LOOP, RETURN_JUMP } type;

  union {
    // Declaration
    struct {
      struct DeclAstNode_t* decl;
    } decl;

    // Expression
    struct {
      struct ExprAstNode_t* expr;
    } expr;

    // Block statement
    struct {
      struct StmtAstNode_t* head;
    } block;

    // While statement
    struct {
      struct ExprAstNode_t* expr;
      struct StmtAstNode_t* block;
    } while_loop;
     
    // Return statement
    struct {
      struct ExprAstNode_t* value;
    } return_jump;
  };

  struct StmtAstNode_t* next;
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
