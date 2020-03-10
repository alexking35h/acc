#ifndef __SCANNER__
#define __SCANNER__

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#include "token.h"
#include "scanner.h"

#define TOKEN_BUFFER_SIZE 32

#define count(n) sizeof(n)/sizeof(n[0])

/* Tokens are allocated in blocks of TOKEN_BUFFER_SIZE,
and stored in a linked list so that new tokens can be 
created on-demand. */
typedef struct TokenList_t
{
    // Linked-list pointer
    struct TokenList_t * next;

    Token tokens[TOKEN_BUFFER_SIZE];
} TokenList;

/* Scanner instance */
typedef struct Scanner_t
{
    // Source file
    char * source;
   
    // Current position in the input. 
    int current;

    // Allocated tokens
    TokenList * token_list;
    Token * next_token;
    int token_count;
} Scanner;

static Token * get_token(Scanner * scanner)
{
    if((scanner->token_count++ == TOKEN_BUFFER_SIZE) != 0)
    {
        return scanner->next_token++;
    }
    else
    {
        // Allocate more space for tokens.
        TokenList * token_list = scanner->token_list;
        while(token_list->next)
        {
            token_list = token_list->next;
        }
        token_list->next = (TokenList *)calloc(1, sizeof(TokenList));
        scanner->next_token = &token_list->next->tokens[0];
        scanner->token_count = 0;
    }
    return NULL;
}

/* Return true if the source character is whitespace */
static bool whitespace(Scanner * scanner, char source)
{
    char whitespace[] = {'\n', '\t', '\v', '\f', ' '};
    for(int i=0;i<count(whitespace);i++)
    {
        if(source == whitespace[i])
        {
            return true;
        }
    }
    return false;
}

static TokenType get_next_token_type(Scanner * scanner)
{
    // Skip over whitespace.
    while(whitespace(scanner, scanner->source[scanner->current]))
    {
        scanner->current++;
    }
    char focus = scanner->source[scanner->current++];

    switch(focus)
    {
        case ';':
            return SEMICOLON;
        case '{':
            return LEFT_BRACE;
        case '}':
            return RIGHT_BRACE;
        case ',':
            return COMMA;
        case ':':
            return COLON;
        case '(':
            return LEFT_PAREN;
        case ')':
            return RIGHT_PAREN;  
        case '[':   
            return LEFT_SQUARE;
        case ']':
            return RIGHT_SQUARE;
        case '~':
            return TILDE;
        case '?':
            return QUESTION;
    }
    return 0;
}


/*
 * Initialize Scanner
 *
 * This function initializes an instance of the Scanner for a given source 
 * file. When finished, call the destructor function (Scanner_destroy)
 *
 * Parameters:
 *  source_file - path to source file
 *
 * Returns:
 *  pointer to allocated Scanner instance, or NULL on error.
 */
Scanner * Scanner_init(char const * source_file)
{
    Scanner * scanner = malloc(sizeof(Scanner));

    // Allocate the first block of tokens.
    scanner->token_list = (TokenList *)calloc(1, sizeof(TokenList));
    scanner->token_count = 0;
    scanner->next_token = &scanner->token_list->tokens[0];

    return scanner;
}

/*
 * Get the next token
 *
 * This function generates the next token from the source file, including the
 * final EOF token. When finished, the function returns NULL.
 */
Token * Scanner_get_next(Scanner * scanner)
{
    return get_token(scanner);
}

/*
 * Destroy instance of ACC's Scanner.
 */
void Scanner_destroy(Scanner * scanner)
{
    // Free up tokens.
    TokenList * token_list = scanner->token_list;
    while(token_list)
    {
        TokenList * p = token_list->next;
        free(token_list);
        token_list = p;
    }
    
    // Free up scanner.
    free(scanner);
}

int main()
{
    Scanner * scanner = Scanner_init("sdf");

    get_token(scanner);
    get_token(scanner);
    Scanner_destroy(scanner);
}

#endif
