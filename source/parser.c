#include "parser.h"

#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ast.h"
#include "scanner.h"
#include "token.h"

/*
 * Initialize the Parser instance.
 */
Parser *Parser_init(Scanner *scanner, ErrorReporter *error_reporter)
{
    Parser *parser = calloc(1, sizeof(Parser));
    parser->scanner = scanner;
    parser->error_reporter = error_reporter;

    parser->next_token[0] = Scanner_get_next(parser->scanner);
    parser->next_token[1] = Scanner_get_next(parser->scanner);
    parser->next_token_index = 0;

    return parser;
}

/*
 * Destroy the Parser instance.
 */
void Parser_destroy(Parser *parser)
{
    free(parser);
}

/*
 * If the next token matches any of the tokens 'token_types', advance the
 * token stream, and return the token. 'token_types' is a NULL terminated
 * array of TokenType.
 */
Token *Parser_match_token(Parser *parser, TokenType *token_types)
{
    for (; *token_types != NAT; token_types++)
    {
        Token *t = Parser_peek_token(parser);
        if (t->type != *token_types)
            continue;

        Parser_advance_token(parser);
        return t;
    }
    return NULL;
}

/*
 * Check out the next token.
 */
Token *Parser_peek_token(Parser *parser)
{
    return parser->next_token[parser->next_token_index];
}

/*
 * Check out the next+1 token.
 */
Token *Parser_peek_next_token(Parser *parser)
{
    int ind = (parser->next_token_index + 1) % 2;
    return parser->next_token[ind];
}

/*
 * If the next token does not match, report an error and return false.
 */
Token *Parser_consume_token(Parser *parser, TokenType token_type)
{
    Token *tok = Parser_match_token(parser, (TokenType[]){token_type, NAT});
    if (!tok)
    {
        Token *tok = Parser_peek_token(parser);

        char err_msg[80];
        snprintf(err_msg, 80, "Expecting '%s', got '%s'", Token_str(token_type),
                 Token_str(tok->type));

        Error_report_error(parser->error_reporter, PARSER, tok->line_number,
                           tok->line_position, err_msg, "");
        THROW_ERROR(parser);
    }
    return tok;
}

/*
 * Advance the token input.
 */
void Parser_advance_token(Parser *parser)
{
    parser->next_token[parser->next_token_index] = Scanner_get_next(parser->scanner);
    parser->next_token_index = (parser->next_token_index + 1) % 2;
}

/*
 * Create a new token, for the purposes of desugauring syntax.
 *
 * This function allocates a new token (using the Scanner_create_token
 * function), and initializes it.
 */
Token *Parser_create_fake_token(Parser *parser, TokenType type, char *lexeme)
{
    Token *token = Scanner_create_token(parser->scanner);

    token->type = type;
    token->line_number = -1;
    token->lexeme = calloc(1, strlen(lexeme) + 1);
    strcpy(token->lexeme, lexeme);

    return token;
}

void Parser_sync_token(Parser *parser, TokenType types[])
{
    while (1)
    {
        for (TokenType *t = &types[0]; *t != NAT; t++)
        {
            if (Parser_peek_token(parser)->type == *t)
                return;
        }
        Parser_advance_token(parser);
    }
}