#ifndef __AST__
#define __AST__

#include "token.h"

enum AstNodeType { BINARY, UNARY, PRIMARY, POSTFIX };

enum AstPrimaryNodeType {
  PRIMARY_IDENTIFIER,
  PRIMARY_CONSTANT,
  PRIMARY_STRING_LITERAL,
};

enum AstPostfixNodeType { POSTFIX_OP, POSTFIX_ARRAY_INDEX };

typedef struct AstNode_t {
  // Type of this AST node.
  enum AstNodeType type;

  union {
    struct {
      struct AstNode_t* left;
      Token* op;
      struct AstNode_t* right;
    } binary;
    struct {
      Token* op;
      struct AstNode_t* right;
    } unary;
    struct {
      enum AstPrimaryNodeType type;
      Token* identifier;
      Token* constant;
      Token* string_literal;
    } primary;
    struct {
      enum AstPostfixNodeType type;
      Token* op;
      struct AstNode_t* index_expression;
      struct AstNode_t* left;
    } postfix;
  } node;
} AstNode;

#define AST_CREATE_BINARY(...) \
  Ast_create_node((AstNode){.type = BINARY, .node.binary = {__VA_ARGS__}})
#define AST_CREATE_UNARY(...) \
  Ast_create_node((AstNode){.type = UNARY, .node.unary = {__VA_ARGS__}})
#define AST_CREATE_PRIMARY(...) \
  Ast_create_node((AstNode){.type = PRIMARY, .node.primary = {__VA_ARGS__}})
#define AST_CREATE_POSTFIX(...) \
  Ast_create_node((AstNode){.type = POSTFIX, .node.postfix = {__VA_ARGS__}})

AstNode* Ast_create_node(AstNode ast_node);
int Ast_pretty_print(AstNode* ast_node, char* buffer, int len);

#endif
