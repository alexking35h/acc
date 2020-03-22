#include <stdlib.h>

#include "ast.h"
#include "error.h"
#include "parser.h"

AstNode* Parser_expression(Parser* parser) {
  // expression -> assignment_expression
  //             | expression, assignment_expression
  return Parser_assignment_expression(parser);
}

AstNode* Parser_assignment_expression(Parser* parser) {
  // assignment_expression -> conditional_expression
  //                       |  unary_expression assignment_operator
  //                       assignment_expression
  return Parser_conditional_expression(parser);
}

AstNode* Parser_conditional_expression(Parser* parser) {
  // conditional_expression -> logical_or_expression ? expression :
  // logical_or_expression
  //                        |  logical_or_expression
  return Parser_logical_or_expression(parser);
}

AstNode* Parser_logical_or_expression(Parser* parser) {
  // logical_or_expression -> logical_and_expression
  //                       |  logical_or_expression OR_OP logical_and_expression
  return Parser_logical_and_expression(parser);
}

AstNode* Parser_logical_and_expression(Parser* parser) {
  // logical_and_expression -> inclusive_or_expression
  //                        |  logical_and_expression AND_OP
  //                        inclusive_or_expression
  // (Todo, for now:)
  // logical_and_expression -> unary_expression
  //                        |  logical_and_expression AND_OP unary_expression
  return Parser_unary_expression(parser);
}

AstNode* Parser_unary_expression(Parser* parser) {
  // unary_expression -> postfix_expression
  //                  |  INC_OP unary_expression
  //                  |  DEC_OP unary_expression
  //                  |  unary_operator cast_expression
  //                  |  sizeof unary_expression
  //                  |  sizeof '(' type_name ')'
  //                  |  alignof '(' type_name ')'
  Token * next_token;
  if((next_token = Parser_match_token(parser, INC_OP)))
    return AST_CREATE_UNARY(.op=next_token, .right=Parser_unary_expression(parser));

  else if((next_token = Parser_match_token(parser, DEC_OP)))
    return AST_CREATE_UNARY(.op=next_token, .right=Parser_unary_expression(parser));

  else if ((next_token = Parser_match_token(parser, SIZEOF)))
    return AST_CREATE_UNARY(.op=next_token, .right=Parser_unary_expression(parser));

  else return Parser_postfix_expression(parser);
}

AstNode* Parser_postfix_expression(Parser* parser) {
  // postfix_expression -> primary_expression
  //                    |  postfix_expression '[' expression ']'
  //                    |  postfix_expression '(' ')'
  //                    |  postfix_expression '(' argument_expression_list ')'
  //                    |  postfix_expression '.' IDENTIFIER
  //                    |  postfix_expression PTR_OP IDENTIFIER
  //                    |  postfix_expression INC_OP
  //                    |  postfix_expression DEC_OP
  //                    |  '(' type_name ')' '{' initializer_list '}'
  //                    |  '(' type_name ')' '{' initializer_list ',' '}'
  AstNode* expr = Parser_primary_expression(parser);

  while (true) {
    Token* next_token;
    if ((next_token = Parser_match_token(parser, LEFT_SQUARE))) {
      AstNode* index_expr = Parser_expression(parser);
      Parser_consume_token(parser, RIGHT_SQUARE);
      expr = AST_CREATE_POSTFIX(.type = POSTFIX_ARRAY_INDEX,
                                .index_expression = index_expr, .left = expr);
    }

    else if ((next_token = Parser_match_token(parser, DOT)))
      expr = AST_CREATE_POSTFIX(.type = POSTFIX_OP, .op = next_token,
                                .left = expr);

    else if ((next_token = Parser_match_token(parser, PTR_OP)))
      expr = AST_CREATE_POSTFIX(.type = POSTFIX_OP, .op = next_token,
                                .left = expr);

    else if ((next_token = Parser_match_token(parser, INC_OP)))
      expr = AST_CREATE_POSTFIX(.type = POSTFIX_OP, .op = next_token,
                                .left = expr);

    else if ((next_token = Parser_match_token(parser, DEC_OP)))
      expr = AST_CREATE_POSTFIX(.type = POSTFIX_OP, .op = next_token,
                                .left = expr);

    else
      break;
  }
  return expr;
}

AstNode* Parser_primary_expression(Parser* parser) {
  // primary_expression -> IDENTIFIER
  //                    |  CONSTANT
  //                    |  STRING_LITERAL
  //                    |  '(' expression ')'

  Token* next_token = Parser_peek_token(parser);
  if (Parser_match_token(parser, IDENTIFIER))
    return AST_CREATE_PRIMARY(.type = PRIMARY_IDENTIFIER,
                              .identifier = next_token);

  if (Parser_match_token(parser, CONSTANT))
    return AST_CREATE_PRIMARY(.type = PRIMARY_CONSTANT, .constant = next_token);

  if (Parser_match_token(parser, STRING_LITERAL))
    return AST_CREATE_PRIMARY(.type = PRIMARY_STRING_LITERAL,
                              .string_literal = next_token);

  if (Parser_match_token(parser, LEFT_PAREN)) {
    AstNode* expression = Parser_expression(parser);
    Parser_consume_token(parser, RIGHT_PAREN);
    return expression;
  }

  return NULL;
}
