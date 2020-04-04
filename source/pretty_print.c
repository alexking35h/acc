#include <stdio.h>
#include <string.h>

#include "ast.h"
#include "pretty_print.h"

#define pp_printf(buf, ...) \
  buf->start += snprintf(buf->start, buf->end - buf->start, __VA_ARGS__);

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
static void pp_decl(DeclAstNode* node, StringBuffer* buf);

static void pp_type(CType* type, StringBuffer* buf);
static void pp_type_primitive(CType* type, StringBuffer* buf);

/*
 * Generate a string for the given ExprAstNode.
 */
int pretty_print_expr(ExprAstNode* node, char* buf, int len) {
  StringBuffer str_buf = {buf, buf + len};
  pp_expr(node, &str_buf);
  return str_buf.end - str_buf.start;
}

/*
 * Generate a string for the given DeclAstNode.
 */
int pretty_print_decl(DeclAstNode* node, char* buf, int len) {
  StringBuffer str_buf = {buf, buf + len};
  pp_decl(node, &str_buf);
  return str_buf.end - str_buf.start;
}

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
      break;

    case ASSIGN:
      pp_assign(node, buf);
      break;
  }
  pp_printf(buf, ")");
}

static void pp_primary(ExprAstNode* node, StringBuffer* buf) {
  pp_printf(buf, "P ");

  if (node->primary.identifier) {
    pp_printf(buf, "%s", node->primary.identifier->lexeme);
  } else if (node->primary.constant) {
    pp_printf(buf, "%s", node->primary.constant->lexeme);
  } else if (node->primary.string_literal) {
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

static void pp_decl(DeclAstNode* node, StringBuffer* buf) {
  pp_printf(buf, "(D ");
  pp_type(node->type, buf);

  if (node->identifier) {
    pp_printf(buf, ", %s", node->identifier->lexeme);
  }

  pp_printf(buf, ")");
}

static void pp_type(CType* type, StringBuffer* buf) {
  pp_printf(buf, "[");

  switch (type->type) {
    case TYPE_PRIMITIVE:
      pp_type_primitive(type, buf);
      break;

    case TYPE_ARRAY:
      pp_printf(buf, "[%d] ", type->array.size);
      pp_type(type->array.type, buf);
      break;

    case TYPE_POINTER:
      pp_printf(buf, "* ");
      pp_type(type->pointer.target, buf);
      break;
  }
  pp_printf(buf, "]");
}

static void pp_type_primitive(CType* type, StringBuffer* buf) {
  // storage specifier first.
  switch (type->primitive.storage_class_specifier) {
    case TYPE_EXTERN:
      pp_printf(buf, "extern ");
      break;
    case TYPE_STATIC:
      pp_printf(buf, "static ");
      break;
    case TYPE_AUTO:
      pp_printf(buf, "auto ");
      break;
    case TYPE_REGISTER:
      pp_printf(buf, "register ");
      break;
  }

  // Qualifiers
  switch (type->primitive.type_qualifier) {
    case TYPE_CONST:
      pp_printf(buf, "const ");
      break;
    case TYPE_VOLATILE:
      pp_printf(buf, "volatile ");
      break;
  }

  // Type
  if (type->primitive.type_specifier & TYPE_SIGNED) {
    pp_printf(buf, "signed ");
  } else if (type->primitive.type_specifier & TYPE_UNSIGNED) {
    pp_printf(buf, "unsigned ");
  }

  if (type->primitive.type_specifier & TYPE_SHORT) {
    pp_printf(buf, "short ");
  } else if (type->primitive.type_specifier & TYPE_LONG) {
    pp_printf(buf, "long ");
  }

  if (type->primitive.type_specifier & TYPE_VOID) {
    pp_printf(buf, "void");
  } else if (type->primitive.type_specifier & TYPE_CHAR) {
    pp_printf(buf, "char");
  } else if (type->primitive.type_specifier & TYPE_INT) {
    pp_printf(buf, "int");
  }
}