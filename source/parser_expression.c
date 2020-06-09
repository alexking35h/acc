/* 
 * Recursive Descent Parser implementation (expression)
 *
 * The recursive descent parser is implemented as a set of mutually recursive
 * functions for each rule in the grammar. This file provides functions for
 * rules in the 'expression' set.
 */

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "ast.h"
#include "parser.h"
#include "token.h"

#define EXPR_BINARY(l, p, ...)                                                           \
    Ast_create_expr_node((ExprAstNode){BINARY, l, p, .binary = {__VA_ARGS__}})
#define EXPR_UNARY(l, p, ...)                                                            \
    Ast_create_expr_node((ExprAstNode){UNARY, l, p, .unary = {__VA_ARGS__}})
#define EXPR_PRIMARY(l, p, ...)                                                          \
    Ast_create_expr_node((ExprAstNode){PRIMARY, l, p, .primary = {__VA_ARGS__}})
#define EXPR_POSTFIX(l, p, ...)                                                          \
    Ast_create_expr_node((ExprAstNode){POSTFIX, l, p, .postfix = {__VA_ARGS__}})
#define EXPR_CAST(l, p, ...)                                                             \
    Ast_create_expr_node((ExprAstNode){CAST, l, p, .cast = {__VA_ARGS__}})
#define EXPR_TERTIARY(l, p, ...)                                                         \
    Ast_create_expr_node((ExprAstNode){TERTIARY, l, p, .tertiary = {__VA_ARGS__}})
#define EXPR_ASSIGN(l, p, ...)                                                           \
    Ast_create_expr_node((ExprAstNode){ASSIGN, l, p, .assign = {__VA_ARGS__}})

#define match(...) Parser_match_token(parser, (TokenType[]){__VA_ARGS__, NAT})
#define consume(t) Parser_consume_token(parser, t)
#define peek() Parser_peek_token(parser)

#define static

static ExprAstNode *primary_expression(Parser *);
static ExprAstNode *postfix_expression(Parser *);
static ArgumentListItem *argument_expression_list(Parser *);
static ExprAstNode *unary_expression(Parser *);
static ExprAstNode *cast_expression(Parser *);
static ExprAstNode *multiplicative_expression(Parser *);
static ExprAstNode *additive_expression(Parser *);
static ExprAstNode *shift_expression(Parser *);
static ExprAstNode *relational_expression(Parser *);
static ExprAstNode *equality_expression(Parser *);
static ExprAstNode *and_expression(Parser *);
static ExprAstNode *exclusive_or_expression(Parser *);
static ExprAstNode *inclusive_or_expression(Parser *);
static ExprAstNode *logical_and_expression(Parser *);
static ExprAstNode *logical_or_expression(Parser *);
static ExprAstNode *conditional_expression(Parser *);

static ExprAstNode *desugar_assign(Parser *parser, ExprAstNode *expr, TokenType op,
                                   ExprAstNode *operand)
{
    /* A bunch of the C syntax is treated as syntactic sugar, to make the AST more
     * homogeneous and (hopefully) make it easier to implement later parts of the
     * compiler. This includes ++ and -- unary operators, and assignment
     * operators (+=, -=, /=, etc.)
     */
    char *op_tok_str;
    switch (op)
    {
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
    Token *op_token = Parser_create_fake_token(parser, op, op_tok_str);
    ExprAstNode *op_expr = EXPR_BINARY(expr->line_number, expr->line_position,
                                       .left = expr, .op = op_token, .right = operand);

    return EXPR_ASSIGN(expr->line_number, expr->line_position, .left = expr,
                       .right = op_expr);
}

static ExprAstNode *desugar_array(Parser *parser, ExprAstNode *base, ExprAstNode *index)
{
    Token *op_plus = Parser_create_fake_token(parser, PLUS, "+");
    Token *op_star = Parser_create_fake_token(parser, STAR, "*");
    ExprAstNode *binary_op =
        EXPR_BINARY(base->line_number, -1, .left = base, .op = op_plus, .right = index);
    ExprAstNode *ptr_op =
        EXPR_UNARY(base->line_number, -1, .op = op_star, .right = binary_op);
    return ptr_op;
}

static ExprAstNode *primary_expression(Parser *parser)
{
    Token *next = match(IDENTIFIER, CONSTANT, STRING_LITERAL, LEFT_PAREN);

    if (!next)
    {
        char err_str[100];
        snprintf(err_str, 100, "Expected expression, got '%s'", Token_str(peek()->type));
        Error_report_error(parser->error_reporter, PARSER, peek()->line_number,
                           peek()->line_position, err_str);

        THROW_ERROR(parser);

        return NULL;
    }
    int l = next->line_number, p = next->line_position;

    if (next->type == IDENTIFIER)
    {
        return EXPR_PRIMARY(l, p, .identifier = next);
    }
    else if (next->type == CONSTANT)
    {
        return EXPR_PRIMARY(l, p, .constant = next);
    }
    else if (next->type == STRING_LITERAL)
    {
        return EXPR_PRIMARY(l, p, .string_literal = next);
    }
    else if (next->type == LEFT_PAREN)
    {
        ExprAstNode *expr = Parser_expression(parser);
        consume(RIGHT_PAREN);
        return expr;
    }
    return NULL;
}

static ExprAstNode *postfix_expression(Parser *parser)
{
    ExprAstNode *expr = primary_expression(parser);
    Token *token;

    while (true)
    {
        if (match(LEFT_SQUARE))
        {
            ExprAstNode *index = Parser_expression(parser);
            consume(RIGHT_SQUARE);
            expr = desugar_array(parser, expr, index);
        }
        else if ((token = match(INC_OP, DEC_OP)))
        {
            expr = EXPR_POSTFIX(token->line_number, token->line_position, .op = token,
                                .left = expr);
        }
        else if ((token = match(LEFT_PAREN)))
        {
            // Function call.
            expr = EXPR_POSTFIX(expr->line_number, expr->line_position, .left = expr,
                                .args = argument_expression_list(parser));
            consume(RIGHT_PAREN);
        }
        else
        {
            break;
        }
    }
    return expr;
}
static ArgumentListItem *argument_expression_list(Parser *parser)
{
    if (peek()->type == RIGHT_PAREN)
        return NULL;

    ArgumentListItem *arg = calloc(1, sizeof(ArgumentListItem));
    arg->argument = Parser_expression(parser);
    arg->next = match(COMMA) ? argument_expression_list(parser) : NULL;
    return arg;
}
static ExprAstNode *unary_expression(Parser *parser)
{
    Token *token;

    if ((token = match(AMPERSAND, STAR, PLUS, MINUS, TILDE, BANG, SIZEOF)))
    {
        return EXPR_UNARY(token->line_number, token->line_position, .op = token,
                          .right = unary_expression(parser));
    }
    else if ((token = match(INC_OP, DEC_OP)))
    {
        ExprAstNode *expr = unary_expression(parser);
        Token *constant_token = Parser_create_fake_token(parser, CONSTANT, "1");
        ExprAstNode *constant_node = EXPR_PRIMARY(
            token->line_number, token->line_position, .constant = constant_token);

        return desugar_assign(parser, expr, token->type == INC_OP ? PLUS : MINUS,
                              constant_node);
    }

    return postfix_expression(parser);
}

static ExprAstNode *cast_expression(Parser *parser)
{
    if (peek()->type != LEFT_PAREN)
        return unary_expression(parser);

    // We may be looking at a cast expression, such as:
    // (int)var;
    // We can't know, without looking at the next+1 token, to see
    // if it's a type-specifier/qualifier/storage-specifier.
    switch (Parser_peek_next_token(parser)->type)
    {
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
    CType *type = Parser_type_name(parser);
    consume(RIGHT_PAREN);
    return EXPR_CAST(peek()->line_number, peek()->line_position, .type = type,
                     .right = cast_expression(parser));
}
static ExprAstNode *multiplicative_expression(Parser *parser)
{
    ExprAstNode *expr = cast_expression(parser);
    Token *operator;

    while (true)
    {
        if (NULL == (operator= match(STAR, SLASH, PERCENT)))
            break;

        ExprAstNode *right = cast_expression(parser);
        expr = EXPR_BINARY(operator->line_number, operator->line_position, .left = expr,
                           .op = operator, .right = right);
    }
    return expr;
}
static ExprAstNode *additive_expression(Parser *parser)
{
    ExprAstNode *expr = multiplicative_expression(parser);
    Token *operator;

    while (true)
    {
        if (NULL == (operator= match(PLUS, MINUS)))
            break;

        ExprAstNode *right = multiplicative_expression(parser);
        expr = EXPR_BINARY(operator->line_number, operator->line_position, .left = expr,
                           .op = operator, .right = right);
    }
    return expr;
}
static ExprAstNode *shift_expression(Parser *parser)
{
    ExprAstNode *expr = additive_expression(parser);
    Token *operator;

    while (true)
    {
        if (NULL == (operator= match(LEFT_OP, RIGHT_OP)))
            break;

        ExprAstNode *right = additive_expression(parser);
        expr = EXPR_BINARY(operator->line_number, operator->line_position, .left = expr,
                           .op = operator, .right = right);
    }
    return expr;
}
static ExprAstNode *relational_expression(Parser *parser)
{
    ExprAstNode *expr = shift_expression(parser);
    Token *operator;

    while (true)
    {
        if (NULL == (operator= match(LESS_THAN, GREATER_THAN, LE_OP, GE_OP)))
            break;

        ExprAstNode *right = shift_expression(parser);
        expr = EXPR_BINARY(operator->line_number, operator->line_position, .left = expr,
                           .op = operator, .right = right);
    }
    return expr;
}
static ExprAstNode *equality_expression(Parser *parser)
{
    ExprAstNode *expr = relational_expression(parser);
    Token *operator;

    while (true)
    {
        if (NULL == (operator= match(EQ_OP, NE_OP)))
            break;

        ExprAstNode *right = relational_expression(parser);
        expr = EXPR_BINARY(operator->line_number, operator->line_position, .left = expr,
                           .op = operator, .right = right);
    }
    return expr;
}
static ExprAstNode *and_expression(Parser *parser)
{ // @DONE
    /*
     * equality_expression
     * and_expression '&' equality_expression
     */
    ExprAstNode *expr = equality_expression(parser);
    Token *operator;

    while (true)
    {
        if (NULL == (operator= match(AMPERSAND)))
            break;

        ExprAstNode *right = equality_expression(parser);
        expr = EXPR_BINARY(operator->line_number, operator->line_position, .left = expr,
                           .op = operator, .right = right);
    }
    return expr;
}
static ExprAstNode *exclusive_or_expression(Parser *parser)
{ // @DONE
    /*
     * and_expression
     * exclusive_or_expression '^' and_expression
     */
    ExprAstNode *expr = and_expression(parser);
    Token *operator;

    while (true)
    {
        if (NULL == (operator= match(CARET)))
            break;

        ExprAstNode *right = and_expression(parser);
        expr = EXPR_BINARY(operator->line_number, operator->line_position, .left = expr,
                           .op = operator, .right = right);
    }
    return expr;
}
static ExprAstNode *inclusive_or_expression(Parser *parser)
{ // @DONE
    /*
     * exclusive_or_expression
     * inclusive_or_expression '|' exclusive_or_expression
     */
    ExprAstNode *expr = exclusive_or_expression(parser);
    Token *operator;

    while (true)
    {
        if (NULL == (operator= match(BAR)))
            break;

        ExprAstNode *right = exclusive_or_expression(parser);
        expr = EXPR_BINARY(operator->line_number, operator->line_position, .left = expr,
                           .op = operator, .right = right);
    }
    return expr;
}
static ExprAstNode *logical_and_expression(Parser *parser)
{ // @DONE
    /*
     * inclusive_or_expression
     * logical_and_expression AND_OP inclusive_or_expression
     */
    ExprAstNode *expr = inclusive_or_expression(parser);
    Token *operator;

    while (true)
    {
        if (NULL == (operator= match(AND_OP)))
            break;

        ExprAstNode *right = inclusive_or_expression(parser);
        expr = EXPR_BINARY(operator->line_number, operator->line_position, .left = expr,
                           .op = operator, .right = right);
    }
    return expr;
}
static ExprAstNode *logical_or_expression(Parser *parser)
{ // @DONE
    /*
     * logical_and_expression
     * logical_or_expression OR_OP logical_and_expression
     */
    ExprAstNode *expr = logical_and_expression(parser);
    Token *operator;

    while (true)
    {
        if (NULL == (operator= match(OR_OP)))
            break;

        ExprAstNode *right = logical_and_expression(parser);
        expr = EXPR_BINARY(operator->line_number, operator->line_position, .left = expr,
                           .op = operator, .right = right);
    }
    return expr;
}

static ExprAstNode *conditional_expression(Parser *parser)
{ // @DONE
    /*
     * logical_or_expression
     * logical_or_expression '?' expression ':' conditional_expression
     */
    ExprAstNode *expr = logical_or_expression(parser);
    Token *question = match(QUESTION);
    if (!question)
        return expr;

    ExprAstNode *expr_true = Parser_expression(parser);
    consume(COLON);
    ExprAstNode *expr_false = conditional_expression(parser);

    return EXPR_TERTIARY(question->line_number, question->line_position,
                         .condition_expr = expr, .expr_true = expr_true,
                         .expr_false = expr_false);
}

ExprAstNode *Parser_assignment_expression(Parser *parser)
{ // @DONE
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
    ExprAstNode *expr = conditional_expression(parser);
    Token *operator;

    while (true)
    {
        operator= match(EQUAL);
        if (operator!= NULL)
        {
            ExprAstNode *right = Parser_assignment_expression(parser);
            expr = EXPR_ASSIGN(operator->line_number, operator->line_position,
                               .left = expr, .right = right);
            continue;
        }

        operator= match(MUL_ASSIGN, DIV_ASSIGN, MOD_ASSIGN, ADD_ASSIGN, SUB_ASSIGN,
                        LEFT_ASSIGN, RIGHT_ASSIGN, AND_ASSIGN, XOR_ASSIGN, OR_ASSIGN);

        if (operator!= NULL)
        {
            TokenType tok_type;
            switch (operator->type)
            {
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
            ExprAstNode *right = Parser_assignment_expression(parser);
            expr = desugar_assign(parser, expr, tok_type, right);
        }
        else
        {
            break;
        }
    }
    return expr;
}

ExprAstNode *Parser_expression(Parser *parser)
{ // @DONE
    /*
     * Parser_assignment_expression
     * expression ',' Parser_assignment_expression // @TODO
     */
    return Parser_assignment_expression(parser);
}
