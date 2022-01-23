/*
 * Recursive Descent Parser implementation (statement)
 *
 * The recursive descent parser is implemented as a set of mutually recursive
 * functions for each rule in the grammar. This file provides functions for
 * rules in the 'statement' set.
 */

#include <stddef.h>

#include "ast.h"
#include "parser.h"
#include "token.h"

#define STMT_EXPR(p, ...)                                                                \
    Ast_create_stmt_node((StmtAstNode){.type = EXPR, p, .expr = {__VA_ARGS__}})
#define STMT_DECL(p, ...)                                                                \
    Ast_create_stmt_node((StmtAstNode){.type = DECL, p, .decl = {__VA_ARGS__}})
#define STMT_BLOCK(p, ...)                                                               \
    Ast_create_stmt_node((StmtAstNode){.type = BLOCK, p, .block = {__VA_ARGS__}})
#define STMT_WHILE(p, ...)                                                               \
    Ast_create_stmt_node(                                                                \
        (StmtAstNode){.type = WHILE_LOOP, p, .while_loop = {__VA_ARGS__}})
#define STMT_RETURN(p, ...)                                                              \
    Ast_create_stmt_node(                                                                \
        (StmtAstNode){.type = RETURN_JUMP, p, .return_jump = {__VA_ARGS__}})
#define STMT_IF(p, ...)                                                                  \
    Ast_create_stmt_node(                                                                \
        (StmtAstNode){.type = IF_STATEMENT, p, .if_statement = {__VA_ARGS__}})

static _Bool is_decl(TokenType tok);
static StmtAstNode *statement(Parser *parser);
static StmtAstNode *expression_statement(Parser *parser);
static StmtAstNode *iteration_statement(Parser *parser);
static StmtAstNode *return_statement(Parser *parser);
static StmtAstNode *if_statement(Parser *parser);

static _Bool is_decl(TokenType tok)
{
    switch (tok)
    {
    case VOID:
    case CHAR:
    case SHORT:
    case INT:
    case SIGNED:
    case UNSIGNED:
        return true;
    default:
        return false;
    }
}

static StmtAstNode *statement(Parser *parser)
{
    if (peek()->type == LEFT_BRACE)
    {
        return Parser_compound_statement(parser);
    }
    if (peek()->type == WHILE)
    {
        return iteration_statement(parser);
    }
    if (peek()->type == RETURN)
    {
        return return_statement(parser);
    }
    if (peek()->type == IF)
    {
        return if_statement(parser);
    }
    return expression_statement(parser);
}

StmtAstNode *Parser_compound_statement(Parser *parser)
{
    StmtAstNode *stmt, *head = NULL, **curr = &stmt;

    Token *brace = consume(LEFT_BRACE);
    while (!match(RIGHT_BRACE))
    {
        if (CATCH_ERROR(parser))
        {
            // Error occurred parsing the following declaration.
            // synchronise on the next semi-colon, or until we reach the RIGHT brace.
            sync(SEMICOLON, RIGHT_BRACE, END_OF_FILE);

            if (peek()->type == SEMICOLON)
                advance();
            if (peek()->type == END_OF_FILE)
                return NULL;
            continue;
        }

        if (is_decl(peek()->type))
        {
            DeclAstNode *decl = Parser_declaration(parser);
            *curr = STMT_DECL(decl->pos, .decl = decl);
        }
        else
        {
            *curr = statement(parser);
        }
        if (!head)
            head = *curr;
        curr = &((*curr)->next);
    }
    return STMT_BLOCK(brace->pos, .head = head);
}

static StmtAstNode *expression_statement(Parser *parser)
{
    ExprAstNode *expr = Parser_expression(parser);
    StmtAstNode *stmt = STMT_EXPR(expr->pos, .expr = expr);
    consume(SEMICOLON);
    return stmt;
}

static StmtAstNode *iteration_statement(Parser *parser)
{
    if (match(WHILE))
    {
        consume(LEFT_PAREN);
        ExprAstNode *expr = Parser_expression(parser);
        consume(RIGHT_PAREN);
        StmtAstNode *block = statement(parser);

        return STMT_WHILE(expr->pos, .expr = expr, .block = block);
    }

    return NULL;
}

static StmtAstNode *return_statement(Parser *parser)
{
    StmtAstNode *stmt;
    Token *tok;
    consume(RETURN);

    if ((tok = match(SEMICOLON)))
    {
        stmt = STMT_RETURN(tok->pos);
    }
    else
    {
        ExprAstNode *expr = Parser_expression(parser);
        stmt = STMT_RETURN(expr->pos, .value = expr);
        consume(SEMICOLON);
    }

    return stmt;
}

static StmtAstNode *if_statement(Parser *parser)
{
    /*
     * 'if' '(' expression ')' '{' statement '}'
     * 'if' '(' expression ')' statement 'else' statement
     */
    StmtAstNode *stmt = STMT_IF(consume(IF)->pos);

    consume(LEFT_PAREN);
    stmt->if_statement.expr = Parser_expression(parser);
    consume(RIGHT_PAREN);

    stmt->if_statement.if_arm = statement(parser);

    if (match(ELSE))
    {
        stmt->if_statement.else_arm = statement(parser);
    }

    return stmt;
}
