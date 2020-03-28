#ifndef __AST__
#define __AST__

#include "token.h"

// Macro definitions for creating AST nodes.
// These should be used instead of calling Ast_create_node directly.
#define AST_BINARY(...) \
  Ast_create_node((AstNode){.type = BINARY, .binary = {__VA_ARGS__}})
#define AST_UNARY(...) \
  Ast_create_node((AstNode){.type = UNARY, .unary = {__VA_ARGS__}})
#define AST_PRIMARY(...) \
  Ast_create_node((AstNode){.type = PRIMARY, .primary = {__VA_ARGS__}})
#define AST_POSTFIX(...) \
  Ast_create_node((AstNode){.type = POSTFIX, .postfix = {__VA_ARGS__}})
#define AST_CAST(...) \
  Ast_create_node((AstNode){.type = CAST, .cast = {__VA_ARGS__}})
#define AST_TERTIARY(...) \
  Ast_create_node((AstNode){.type = TERTIARY, .tertiary = {__VA_ARGS__}})
#define AST_ASSIGN(...) \
  Ast_create_node((AstNode){.type = ASSIGN, .assign = {__VA_ARGS__}})
#define AST_EXPR(...) \
  Ast_create_node((AstNode){.type = EXPR, .expr = {__VA_ARGS__}})

// Enum definition for each node type in
// the generated AST.
enum AstNodeType {
  BINARY,
  UNARY,
  PRIMARY,
  POSTFIX,
  CAST,
  TERTIARY,
  ASSIGN,
  EXPR
};

// For 'Primary' Nodes, enum definition for each node sub-type.
enum AstPrimaryNodeType {
  PRIMARY_IDENTIFIER,
  PRIMARY_CONSTANT,
  PRIMARY_STRING_LITERAL,
};

// For 'Postfix' Nodes, enum definition for each node sub-type
enum AstPostfixNodeType { POSTFIX_OP, POSTFIX_ARRAY_INDEX };

// Some rules in the grammar require a flat sequence of AST nodes,
// such as statements in a block, or arguments in a function call.
// These are stored as a flat sequence in a linked list.
typedef struct AstNodeList_t {
  struct AstNodeList_t* next;
  struct AstNode_t* node;
} AstNodeList;

typedef struct AstNode_t {
  // Type of this AST node.
  enum AstNodeType type;

  // Anonymous union for this node.
  union {
    // Binary
    struct {
      struct AstNode_t* left;
      Token* op;
      struct AstNode_t* right;
    } binary;

    // Unary
    struct {
      Token* op;
      struct AstNode_t* right;
    } unary;

    // Primary
    struct {
      enum AstPrimaryNodeType type;
      Token* identifier;
      Token* constant;
      Token* string_literal;
    } primary;

    // Postfix
    struct {
      enum AstPostfixNodeType type;
      Token* op;
      struct AstNode_t* index_expression;
      struct AstNode_t* left;
    } postfix;

    // Cast
    struct {
      Token* type_token;
      struct AstNode_t* right;
    } cast;

    // Tertiary
    struct {
      struct AstNode_t* condition_expr;
      struct AstNode_t* expr_true;
      struct AstNode_t* expr_false;
    } tertiary;

    // Assign
    struct {
      struct AstNode_t* left;
      struct AstNode_t* right;
    } assign;

    // comma-separated expression
    struct {
      struct AstNode_t* expr;
      struct AstNode_t* next;
    } expr;
  };

} AstNode;

/*
 * Create a new AST node
 *
 * This function takes a AstNode by value (presumably allocated automatically),
 * and copies it to a new AstNode allocated dynamically.
 *
 * E.g.:
 * > Ast_create_new((AstNode){.type=...});
 */
AstNode* Ast_create_node(AstNode ast_node);

/*
 * Generate a string for the given AST node.
 *
 * This function generates a string representation for a given AST node.
 */
int Ast_pretty_print(AstNode* ast_node, char* buffer, int buffer_len);

#endif
