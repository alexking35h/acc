#include "pretty_print.h"

#include <stdio.h>
#include <string.h>

#include "ast.h"
#include "ctype.h"

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
static void pp_cast(ExprAstNode* node, StringBuffer* buf);
static void pp_assign(ExprAstNode* node, StringBuffer* buf);
static void pp_decl(DeclAstNode* node, StringBuffer* buf);
static void pp_stmt(StmtAstNode* node, StringBuffer* buf);

static void pp_type(CType* type, StringBuffer* buf);
static void pp_type_primitive(CType* type, StringBuffer* buf);
static void pp_type_function(CType* type, StringBuffer* buf);

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
  if (node) {
    StringBuffer str_buf = {buf, buf + len};
    pp_decl(node, &str_buf);
    return str_buf.end - str_buf.start;
  } else {
    return 0;
  }
}

/*
 * Generate a string for the given StmtAstNode.
 */
int pretty_print_stmt(StmtAstNode* node, char* buf, int len) {
  StringBuffer str_buf = {buf, buf + len};
  pp_stmt(node, &str_buf);
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
      pp_cast(node, buf);
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
  if (node->postfix.op) {
    pp_printf(buf, "PF ");

    pp_expr(node->postfix.left, buf);
    pp_printf(buf, node->postfix.op->type == INC_OP ? ", ++" : ", --");
  } else {
    pp_printf(buf, "F ");
    pp_expr(node->postfix.left, buf);

    for(ArgumentListItem* p = node->postfix.args; p; p = p->next) {
      pp_printf(buf, ", ");
      pp_expr(p->argument, buf);
    }
  }
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

static void pp_cast(ExprAstNode* node, StringBuffer* buf) {
  pp_printf(buf, "C ");
  pp_type(node->cast.type, buf);
  pp_printf(buf, ", ");
  pp_expr(node->cast.right, buf);
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

  if (node->type->type == TYPE_FUNCTION && node->body) {
    pp_printf(buf, ", ");
    pp_stmt(node->body, buf);
  } else if (node->initializer) {
    pp_printf(buf, ", ");
    pp_expr(node->initializer, buf);
  }

  if (node->next) {
    pp_printf(buf, ", ");
    pp_decl(node->next, buf);
  }

  pp_printf(buf, ")");
}

static void pp_stmt(StmtAstNode* node, StringBuffer* buf) {
  if (!node) return;
  pp_printf(buf, "{");

  switch (node->type) {
    case EXPR:
      pp_printf(buf, "E ");
      pp_expr(node->expr.expr, buf);
      break;
    case DECL:
      pp_printf(buf, "D ");
      pp_decl(node->decl.decl, buf);
      break;
    case BLOCK:
      pp_printf(buf, "B ");
      pp_stmt(node->block.head, buf);
      break;
    case WHILE_LOOP:
      pp_printf(buf, "W ");
      pp_expr(node->while_loop.expr, buf);
      pp_printf(buf, ", ");
      pp_stmt(node->while_loop.block, buf);
      break;
    case RETURN_JUMP:
      pp_printf(buf, "R ");
      if(node->return_jump.value)
        pp_expr(node->return_jump.value, buf);
      break;
  }
  if (node->next) {
    pp_printf(buf, ", ");
    pp_stmt(node->next, buf);
  }

  pp_printf(buf, "}");
}

static void pp_type(CType* type, StringBuffer* buf) {
  pp_printf(buf, "[");

  switch (type->type) {
    case TYPE_PRIMITIVE:
      pp_type_primitive(type, buf);
      break;

    case TYPE_ARRAY:
      pp_printf(buf, "[%d] ", type->derived.array_size);
      pp_type(type->derived.type, buf);
      break;

    case TYPE_POINTER:
      pp_printf(buf, "* ");
      pp_type(type->derived.type, buf);
      break;

    case TYPE_FUNCTION:
      pp_type_function(type, buf);
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

static void pp_type_function(CType* type, StringBuffer* buf) {
  pp_printf(buf, "f(");
  for (ParameterListItem* p = type->derived.params; p; p = p->next) {
    pp_type(p->type, buf);

    if (p->name) {
      pp_printf(buf, ":%s", p->name->lexeme);
    } else {
      pp_printf(buf, ":");
    }

    if (p->next) pp_printf(buf, ",");
  }
  pp_printf(buf, ") ");
  pp_type(type->derived.type, buf);
}
