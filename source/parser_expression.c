/* Recursive Descent Parser implementation (expression)
 *
 * The recursive descent parser is implemented as a set of mutually recursive
 * functions for each rule in the grammar. This file provides functions for
 * rules in the 'expression' set.
 *
 * Generated by generate_recursive_parser.py on 2020-03-24.
 */

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "ast.h"
#include "parser.h"
#include "token.h"

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

#define match(...) Parser_match_token(parser, (TokenType[]){__VA_ARGS__, NAT})
#define consume(t) Parser_consume_token(parser, t)
#define peek() Parser_peek_token(parser)

#define static

static ExprAstNode* primary_expression(Parser*);
static ExprAstNode* postfix_expression(Parser*);
static ArgumentListItem* argument_expression_list(Parser*);
static ExprAstNode* unary_expression(Parser*);
static ExprAstNode* cast_expression(Parser*);
static ExprAstNode* multiplicative_expression(Parser*);
static ExprAstNode* additive_expression(Parser*);
static ExprAstNode* shift_expression(Parser*);
static ExprAstNode* relational_expression(Parser*);
static ExprAstNode* equality_expression(Parser*);
static ExprAstNode* and_expression(Parser*);
static ExprAstNode* exclusive_or_expression(Parser*);
static ExprAstNode* inclusive_or_expression(Parser*);
static ExprAstNode* logical_and_expression(Parser*);
static ExprAstNode* logical_or_expression(Parser*);
static ExprAstNode* conditional_expression(Parser*);

static ExprAstNode* desugar_assign(Parser* parser, ExprAstNode* expr,
                                   TokenType op, ExprAstNode* operand) {
  /* A bunch of the C syntax is treated as syntactic sugar, to make the AST more
   * homogeneous and (hopefully) make it easier to implement later parts of the
   * compiler. This includes ++ and -- postfix operators, and assignment
   * operators (+=, -=, /=, etc.)
   */
  char* op_tok_str;
  switch (op) {
    case PLUS:
      op_tok_str = "+";
      break;
    case MINUS:
      op_tok_str = "-";
      break;
    case STAR:
      op_tok_str = "*";
      break;
    case SLASH:
      op_tok_str = "/";
      break;
    case PERCENT:
      op_tok_str = "%";
      break;
    case LEFT_OP:
      op_tok_str = "<<";
      break;
    case RIGHT_OP:
      op_tok_str = ">>";
      break;
    case AMPERSAND:
      op_tok_str = "&";
      break;
    case CARET:
      op_tok_str = "^";
      break;
    case BAR:
      op_tok_str = "|";
      break;
    default:
      op_tok_str = "?";
      break;
  }
  Token* op_token = Parser_create_fake_token(parser, op, op_tok_str);
  ExprAstNode* op_expr =
      EXPR_BINARY(.left = expr, .op = op_token, .right = operand);

  return EXPR_ASSIGN(.left = expr, .right = op_expr);
}

static ExprAstNode *desugar_array(Parser *parser, ExprAstNode* base, ExprAstNode* index) {
  Token* op_plus = Parser_create_fake_token(parser, PLUS, "+");
  Token* op_star = Parser_create_fake_token(parser, STAR, "*");
  ExprAstNode* binary_op = EXPR_BINARY(.left=base, .op=op_plus, .right=index);
  ExprAstNode* ptr_op = EXPR_UNARY(.op=op_star, .right=binary_op);
  return ptr_op;
}

static ExprAstNode* primary_expression(Parser* parser) {  // @DONE
  /*
   * IDENTIFIER
   * constant
   * string
   * '(' expression ')'
   * generic_selection  @TODO
   */
  Token* next;

  if ((next = match(IDENTIFIER))) return EXPR_PRIMARY(.identifier = next);

  if ((next = match(CONSTANT))) return EXPR_PRIMARY(.constant = next);

  if ((next = match(STRING_LITERAL)))
    return EXPR_PRIMARY(.string_literal = next);

  if (match(LEFT_PAREN)) {
    ExprAstNode* expr = Parser_expression(parser);
    consume(RIGHT_PAREN);
    return expr;
  }

  char err_str[100];
  snprintf(err_str, 100, "Expected expression, got '%s'",
           Token_str(peek()->type));
  Error_report_error(PARSER, peek()->line_number, err_str);

  THROW_ERROR(parser);

  return NULL;
}

static ExprAstNode* postfix_expression(Parser* parser) {
  /*
   * primary_expression
   * postfix_expression '[' expression ']'
   * postfix_expression '(' ')'                           @TODO
   * postfix_expression '(' argument_expression_list ')'  @TODO
   * postfix_expression '.' IDENTIFIER
   * postfix_expression PTR_OP IDENTIFIER
   * postfix_expression INC_OP
   * postfix_expression DEC_OP
   * '(' type_name ')' '{' initializer_list '}'           @TODO
   * '(' type_name ')' '{' initializer_list ',' '}'       @TODO
   */
  ExprAstNode* expr = primary_expression(parser);
  Token* token;

  while (true) {
    if (match(LEFT_SQUARE)) {
      ExprAstNode* index = Parser_expression(parser);
      consume(RIGHT_SQUARE);
      expr = desugar_array(parser, expr, index);
    } else if ((token = match(INC_OP))) {
      // INC_OP is desugaured into: a++ -> a=a+1
      Token* constant_token = Parser_create_fake_token(parser, CONSTANT, "1");
      ExprAstNode* constant_node = EXPR_PRIMARY(.constant = constant_token);

      return desugar_assign(parser, expr, PLUS, constant_node);
    } else if ((token = match(DEC_OP))) {
      // DEC_OP is desugaured into: a++ -> a=a-1
      Token* constant_token = Parser_create_fake_token(parser, CONSTANT, "1");
      ExprAstNode* constant_node = EXPR_PRIMARY(.constant = constant_token);

      return desugar_assign(parser, expr, MINUS, constant_node);
    } else if ((token = match(LEFT_PAREN))) {
      // Function call.
      expr = EXPR_POSTFIX(.left=expr, .args = argument_expression_list(parser));
      consume(RIGHT_PAREN);
    }
    else {
      break;
    }
  }
  return expr;
}
static ArgumentListItem* argument_expression_list(Parser* parser) {  // @TODO
  /*
   * Parser_assignment_expression
   * argument_expression_list ',' Parser_assignment_expression
   */
  if(peek()->type == RIGHT_PAREN)
    return NULL;

  ArgumentListItem* arg = calloc(1, sizeof(ArgumentListItem));
  arg->argument = Parser_expression(parser);
  arg->next = match(COMMA) ? argument_expression_list(parser) : NULL;
  return arg;
}
static ExprAstNode* unary_expression(Parser* parser) {  // @DONE
  /*
   * postfix_expression
   * INC_OP unary_expression
   * DEC_OP unary_expression
   * unary_operator cast_expression
   * SIZEOF unary_expression
   * SIZEOF '(' type_name ')'   @TODO
   * ALIGNOF '(' type_name ')'  @TODO
   */
  Token* token;

  if ((token = match(AMPERSAND, STAR, PLUS, MINUS, TILDE, BANG, SIZEOF, INC_OP,
                     DEC_OP)))
    return EXPR_UNARY(.op = token, .right = unary_expression(parser));

  return postfix_expression(parser);
}

static ExprAstNode* cast_expression(Parser* parser) {  // @TODO
  /*
   * unary_expression
   * '(' type_name ')' cast_expression
   */
  if(peek()->type != LEFT_PAREN) return unary_expression(parser);

  // We may be looking at a cast expression, such as:
  // (int)var;
  // We can't know, without looking at the next+1 token, to see
  // if it's a type-specifier/qualifier/storage-specifier.
  switch(Parser_peek_next_token(parser)->type) {
    case VOID:
    case CHAR:
    case SHORT:
    case INT:
    case LONG:
    case FLOAT:
    case DOUBLE:
    case SIGNED:
    case UNSIGNED:
      break;
    default:
      return unary_expression(parser);
  }

  // Cast expression.
  consume(LEFT_PAREN);
  CType* type = Parser_type_name(parser);
  consume(RIGHT_PAREN);
  return EXPR_CAST(.type = type, .right = cast_expression(parser));
}
static ExprAstNode* multiplicative_expression(Parser* parser) {  // @DONE
  /*
   * cast_expression
   * multiplicative_expression '*' cast_expression
   * multiplicative_expression '/' cast_expression
   * multiplicative_expression '%' cast_expression
   */
  ExprAstNode* expr = cast_expression(parser);
  Token* operator;

  while (true) {
    if (NULL == (operator= match(STAR, SLASH, PERCENT))) break;

    ExprAstNode* right = cast_expression(parser);
    expr = EXPR_BINARY(.left = expr, .op = operator, .right = right);
  }
  return expr;
}
static ExprAstNode* additive_expression(Parser* parser) {  // @DONE
  /*
   * multiplicative_expression
   * additive_expression '+' multiplicative_expression
   * additive_expression '-' multiplicative_expression
   */
  ExprAstNode* expr = multiplicative_expression(parser);
  Token* operator;

  while (true) {
    if (NULL == (operator= match(PLUS, MINUS))) break;

    ExprAstNode* right = multiplicative_expression(parser);
    expr = EXPR_BINARY(.left = expr, .op = operator, .right = right);
  }
  return expr;
}
static ExprAstNode* shift_expression(Parser* parser) {  // @DONE
  /*
   * additive_expression
   * shift_expression LEFT_OP additive_expression
   * shift_expression RIGHT_OP additive_expression
   */
  ExprAstNode* expr = additive_expression(parser);
  Token* operator;

  while (true) {
    if (NULL == (operator= match(LEFT_OP, RIGHT_OP))) break;

    ExprAstNode* right = additive_expression(parser);
    expr = EXPR_BINARY(.left = expr, .op = operator, .right = right);
  }
  return expr;
}
static ExprAstNode* relational_expression(Parser* parser) {  // @DONE
  /*
   * shift_expression
   * relational_expression '<' shift_expression
   * relational_expression '>' shift_expression
   * relational_expression LE_OP shift_expression
   * relational_expression GE_OP shift_expression
   */
  ExprAstNode* expr = shift_expression(parser);
  Token* operator;

  while (true) {
    if (NULL == (operator= match(LESS_THAN, GREATER_THAN, LE_OP, GE_OP))) break;

    ExprAstNode* right = shift_expression(parser);
    expr = EXPR_BINARY(.left = expr, .op = operator, .right = right);
  }
  return expr;
}
static ExprAstNode* equality_expression(Parser* parser) {  // @DONE
  /*
   * relational_expression
   * equality_expression EQ_OP relational_expression
   * equality_expression NE_OP relational_expression
   */
  ExprAstNode* expr = relational_expression(parser);
  Token* operator;

  while (true) {
    if (NULL == (operator= match(EQ_OP, NE_OP))) break;

    ExprAstNode* right = relational_expression(parser);
    expr = EXPR_BINARY(.left = expr, .op = operator, .right = right);
  }
  return expr;

  return relational_expression(parser);
}
static ExprAstNode* and_expression(Parser* parser) {  // @DONE
  /*
   * equality_expression
   * and_expression '&' equality_expression
   */
  ExprAstNode* expr = equality_expression(parser);
  Token* operator;

  while (true) {
    if (NULL == (operator= match(AMPERSAND))) break;

    ExprAstNode* right = equality_expression(parser);
    expr = EXPR_BINARY(.left = expr, .op = operator, .right = right);
  }
  return expr;
}
static ExprAstNode* exclusive_or_expression(Parser* parser) {  // @DONE
  /*
   * and_expression
   * exclusive_or_expression '^' and_expression
   */
  ExprAstNode* expr = and_expression(parser);
  Token* operator;

  while (true) {
    if (NULL == (operator= match(CARET))) break;

    ExprAstNode* right = and_expression(parser);
    expr = EXPR_BINARY(.left = expr, .op = operator, .right = right);
  }
  return expr;
}
static ExprAstNode* inclusive_or_expression(Parser* parser) {  // @DONE
  /*
   * exclusive_or_expression
   * inclusive_or_expression '|' exclusive_or_expression
   */
  ExprAstNode* expr = exclusive_or_expression(parser);
  Token* operator;

  while (true) {
    if (NULL == (operator= match(BAR))) break;

    ExprAstNode* right = exclusive_or_expression(parser);
    expr = EXPR_BINARY(.left = expr, .op = operator, .right = right);
  }
  return expr;
}
static ExprAstNode* logical_and_expression(Parser* parser) {  // @DONE
  /*
   * inclusive_or_expression
   * logical_and_expression AND_OP inclusive_or_expression
   */
  ExprAstNode* expr = inclusive_or_expression(parser);
  Token* operator;

  while (true) {
    if (NULL == (operator= match(AND_OP))) break;

    ExprAstNode* right = inclusive_or_expression(parser);
    expr = EXPR_BINARY(.left = expr, .op = operator, .right = right);
  }
  return expr;
}
static ExprAstNode* logical_or_expression(Parser* parser) {  // @DONE
  /*
   * logical_and_expression
   * logical_or_expression OR_OP logical_and_expression
   */
  ExprAstNode* expr = logical_and_expression(parser);
  Token* operator;

  while (true) {
    if (NULL == (operator= match(OR_OP))) break;

    ExprAstNode* right = logical_and_expression(parser);
    expr = EXPR_BINARY(.left = expr, .op = operator, .right = right);
  }
  return expr;
}

static ExprAstNode* conditional_expression(Parser* parser) {  // @DONE
  /*
   * logical_or_expression
   * logical_or_expression '?' expression ':' conditional_expression
   */
  ExprAstNode* expr = logical_or_expression(parser);
  if (!match(QUESTION)) return expr;

  ExprAstNode* expr_true = Parser_expression(parser);
  consume(COLON);
  ExprAstNode* expr_false = conditional_expression(parser);

  return EXPR_TERTIARY(.condition_expr = expr, .expr_true = expr_true,
                       .expr_false = expr_false);
}

ExprAstNode* Parser_assignment_expression(Parser* parser) {  // @DONE
  /*
   * conditional_expression
   * unary_expression assignment_operator Parser_assignment_expression
   */

  // The FIRST sets for the grammar rules 'conditional_expression' and
  // 'unary_expression' are not disjoint. To sidestep this,
  // 'Parser_assignment_expression is parsed as: : conditional_expression |
  // conditional_expression assignment_operator conditional_expression.
  //
  // Later we'll come back to make sure the lvalue is valid.
  ExprAstNode* expr = conditional_expression(parser);
  Token* operator;

  while (true) {
    operator= match(EQUAL);
    if (operator!= NULL) {
      ExprAstNode* right = Parser_assignment_expression(parser);
      expr = EXPR_ASSIGN(.left = expr, .right = right);
      continue;
    }

    operator=
        match(MUL_ASSIGN, DIV_ASSIGN, MOD_ASSIGN, ADD_ASSIGN, SUB_ASSIGN,
              LEFT_ASSIGN, RIGHT_ASSIGN, AND_ASSIGN, XOR_ASSIGN, OR_ASSIGN);

    if (operator!= NULL) {
      TokenType tok_type;
      switch (operator->type) {
        case MUL_ASSIGN:
          tok_type = STAR;
          break;
        case DIV_ASSIGN:
          tok_type = SLASH;
          break;
        case MOD_ASSIGN:
          tok_type = PERCENT;
          break;
        case ADD_ASSIGN:
          tok_type = PLUS;
          break;
        case SUB_ASSIGN:
          tok_type = MINUS;
          break;
        case LEFT_ASSIGN:
          tok_type = LEFT_OP;
          break;
        case RIGHT_ASSIGN:
          tok_type = RIGHT_OP;
          break;
        case AND_ASSIGN:
          tok_type = AMPERSAND;
          break;
        case XOR_ASSIGN:
          tok_type = CARET;
          break;
        case OR_ASSIGN:
          tok_type = BAR;
          break;
        default:
          break;
      }
      ExprAstNode* right = Parser_assignment_expression(parser);
      expr = desugar_assign(parser, expr, tok_type, right);
    } else {
      break;
    }
  }
  return expr;
}

ExprAstNode* Parser_expression(Parser* parser) {  // @DONE
  /*
   * Parser_assignment_expression
   * expression ',' Parser_assignment_expression // @TODO
   */
  return Parser_assignment_expression(parser);
}
