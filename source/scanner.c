#include "scanner.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "error.h"
#include "token.h"

#define TOKEN_BUFFER_SIZE 32
#define IDENTIFIER_START_CHARACTERS                                                      \
    "QWERTYUIOPASDFGHJKLZXCVBNMqwertyuiopasdfghjklzxcvbnm_"
#define IDENTIFIER_CHARACTERS                                                            \
    "QWERTYUIOPASDFGHJKLZXCVBNMqwertyuiopasdfghjklzxcvbnm_1234567890"

#define MAX_KEYWORD_LENGTH 20

#define COUNT(n) sizeof(n) / sizeof(n[0])
#define PEEK(scanner)                                                                    \
    (scanner->current < strlen(scanner->source) ? scanner->source[scanner->current] : 0)
#define END_OF_FILE(scanner) (scanner->current >= strlen(scanner->source))
#define ADVANCE(scanner) (scanner->current++)

/*
 * Scanner class
 * This struct encapsulates all state required by the scanner.
 */
typedef struct Scanner_t
{
    ErrorReporter *error_reporter;

    // Source file
    const char *source;

    // Current position in the input.
    int current;
    int line_number;
    int line_start_position;

    // Line position pointers.
    int line_positions_size;
    int line_positions_next;
    const char **line_positions;
} Scanner;

/*
 * Helper functions:
 * match_*   - conditionally advance the scanner position if the input matches
 * consume_* - report error if the scanner input does not match the expected
 * token doesn't match
 */
static bool match_character(Scanner *, const char *expected);
static bool match_whitespace(Scanner *);
static bool consume_keyword_or_identifier(Scanner *, TokenType *);
static bool consume_number(Scanner *);
static bool consume_string(Scanner *);
static bool consume_comment(Scanner *);

static void store_line_position(Scanner *scanner)
{
    if (scanner->line_positions_next >= scanner->line_positions_size)
    {
        scanner->line_positions = realloc(
            scanner->line_positions, sizeof(char *) * (scanner->line_positions_size + 5));
        scanner->line_positions_size += 5;
    }
    scanner->line_positions[scanner->line_positions_next++] =
        scanner->source + scanner->current;
}

static bool match_character(Scanner *scanner, const char *expected)
{
    if (END_OF_FILE(scanner))
        return false;

    for (; *expected; expected++)
    {
        if (scanner->source[scanner->current] == *expected)
        {
            scanner->current++;
            return true;
        }
    }
    return false;
}

static bool match_whitespace(Scanner *scanner)
{
    char whitespace[] = {'\t', '\v', '\f', ' '};
    char focus = scanner->source[scanner->current];
    if (focus == '\n')
    {
        scanner->line_number++;
        scanner->line_start_position = scanner->current + 1;
        ADVANCE(scanner);
        store_line_position(scanner);
        return true;
    }
    for (int i = 0; i < COUNT(whitespace); i++)
    {
        if (focus == whitespace[i])
        {
            ADVANCE(scanner);
            return true;
        }
    }
    return false;
}

static bool consume_keyword_or_identifier(Scanner *scanner, TokenType *token_type)
{
    const char *identifier_start = scanner->source + scanner->current++;
    int identifier_length = 1;

    while (match_character(scanner, IDENTIFIER_CHARACTERS))
    {
        identifier_length++;
    }

    char *keywords[] = {
        "auto",     "break",  "case",     "char",     "const",    "continue", "default",
        "do",       "else",   "enum",     "extern",   "for",      "goto",     "if",
        "inline",   "int",    "long",     "register", "restrict", "return",   "short",
        "signed",   "sizeof", "static",   "struct",   "switch",   "typedef",  "union",
        "unsigned", "void",   "volatile", "while",
    };

    for (int i = 0; i < COUNT(keywords); i++)
    {
        if (strlen(keywords[i]) != identifier_length)
            continue;
        if (strncmp(identifier_start, keywords[i], identifier_length))
            continue;

        *token_type = AUTO + i;
        return true;
    }
    *token_type = IDENTIFIER;
    return true;
}

static bool consume_string(Scanner *scanner)
{
    int string_line_position = scanner->current - scanner->line_start_position;

    // Skip over the first double-quote
    ADVANCE(scanner);

    while (true)
    {
        if (END_OF_FILE(scanner))
        {
            Position pos = {scanner->line_number, string_line_position};
            Error_report_error(scanner->error_reporter, SCANNER, pos,
                               "Unterminated string literal");
            return false;
        }

        char focus = scanner->source[scanner->current++];

        // End of string, stop searching.
        if (focus == '"')
            break;

        // Reach end of line (before end of string).
        if (focus == '\n')
        {
            Position pos = {scanner->line_number, string_line_position};
            Error_report_error(scanner->error_reporter, SCANNER, pos,
                               "Unterminated string literal");
            scanner->line_number++;
            scanner->line_start_position = scanner->current;
            store_line_position(scanner);
            return false;
        }

        // Escape sequence.
        // These are replaced later. But make sure to ignore the double-quote.
        if (focus == '\\')
        {
            if (match_character(scanner, "\""))
                continue;
        }
    }
    return true;
}

/* Consume a number */
static bool consume_number(Scanner *scanner)
{
    if (match_character(scanner, "0"))
    {
        if (match_character(scanner, "Xx"))
        {
            // Hex notation
            while (match_character(scanner, "1234567890abcdefABCDEF"))
                continue;
        }
        else
        {
            // Oct notation
            while (match_character(scanner, "1234567890"))
                continue;
        }
    }
    else
    {
        while (match_character(scanner, "1234567890"))
            continue;
    }
    match_character(scanner, "ulUL");
    return true;
}

/* Consume a comment */
static bool consume_comment(Scanner *scanner)
{
    ADVANCE(scanner);
    if (match_character(scanner, "/"))
    {
        while (scanner->source[scanner->current] != '\n')
            scanner->current++;
    }
    else if (match_character(scanner, "*"))
    {
        while (true)
        {
            if (match_character(scanner, "\n"))
            {
                scanner->line_number++;
                store_line_position(scanner);
                continue;
            }
            if (scanner->source[scanner->current++] != '*')
                continue;
            if (scanner->source[scanner->current++] != '/')
                continue;
            break;
        }
    }
    return true;
}

static TokenType get_next_token_type(Scanner *scanner)
{
    char focus = scanner->source[scanner->current++];

    // Single-character tokens
    switch (focus)
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
    case '.':
        return DOT;
    case EOF:
        return END_OF_FILE;
    }

    // Operators
    if (focus == '=')
        return match_character(scanner, "=") ? EQ_OP : EQUAL;

    if (focus == '!')
        return match_character(scanner, "=") ? NE_OP : BANG;

    // Assignments
    if (focus == '*')
        return match_character(scanner, "=") ? MUL_ASSIGN : STAR;

    if (focus == '%')
        return match_character(scanner, "=") ? MOD_ASSIGN : PERCENT;

    if (focus == '^')
        return match_character(scanner, "=") ? XOR_ASSIGN : CARET;

    if (focus == '|')
    {
        if (match_character(scanner, "="))
            return OR_ASSIGN;

        if (match_character(scanner, "|"))
            return OR_OP;

        return BAR;
    }

    if (focus == '&')
    {
        if (match_character(scanner, "="))
            return AND_ASSIGN;

        else if (match_character(scanner, "&"))
            return AND_OP;

        else
            return AMPERSAND;
    }

    if (focus == '<')
    {
        if (match_character(scanner, "<"))
        {
            return match_character(scanner, "=") ? LEFT_ASSIGN : LEFT_OP;
        }
        if (match_character(scanner, "="))
            return LE_OP;

        return LESS_THAN;
    }
    if (focus == '>')
    {
        if (match_character(scanner, ">"))
            return match_character(scanner, "=") ? RIGHT_ASSIGN : RIGHT_OP;

        if (match_character(scanner, "="))
            return GE_OP;

        return GREATER_THAN;
    }
    if (focus == '-')
    {
        if (match_character(scanner, "="))
            return SUB_ASSIGN;

        if (match_character(scanner, "-"))
            return DEC_OP;

        if (match_character(scanner, ">"))
            return PTR_OP;

        return MINUS;
    }

    if (focus == '+')
    {
        if (match_character(scanner, "="))
            return ADD_ASSIGN;

        if (match_character(scanner, "+"))
            return INC_OP;

        return PLUS;
    }

    if (focus == '/')
    {
        // Comment.
        if (PEEK(scanner) == '*')
        {
            scanner->current--;
            consume_comment(scanner);
            return NAT;
        }
        else if (PEEK(scanner) == '/')
        {
            scanner->current--;
            consume_comment(scanner);
            return NAT;
        }

        return match_character(scanner, "=") ? DIV_ASSIGN : SLASH;
    }

    // String literal.
    if (focus == '"')
    {
        scanner->current--;
        return consume_string(scanner) ? STRING_LITERAL : NAT;
    }

    // Number literal.
    if (strchr("1234567890", focus))
    {
        scanner->current--;
        return consume_number(scanner) ? CONSTANT : NAT;
    }

    if (strchr(IDENTIFIER_START_CHARACTERS, focus))
    {
        scanner->current--;
        TokenType token;
        return consume_keyword_or_identifier(scanner, &token) ? token : NAT;
    }

    char error_string[50];
    snprintf(error_string, 50, "Invalid character in input: '%c'", focus);

    Position pos = {scanner->line_number,
                    scanner->current - scanner->line_start_position - 1};
    Error_report_error(scanner->error_reporter, SCANNER, pos, error_string);

    return 0;
}

/*
 * Initialize Scanner
 *
 * This function initializes an instance of the Scanner.
 * When finished, call the destructor function (Scanner_destroy)
 *
 * Parameters:
 *  source - pointer to source code
 *
 * Returns:
 *  pointer to allocated Scanner instance, or NULL on error.
 */
Scanner *Scanner_init(char const *source, ErrorReporter *error_reporter)
{
    Scanner *scanner = malloc(sizeof(Scanner));

    scanner->error_reporter = error_reporter;
    scanner->source = source;
    scanner->current = 0;
    scanner->line_number = 1;
    scanner->line_start_position = 0;

    // Allocate space for the line positions array.
    scanner->line_positions = calloc(5, sizeof(char *));
    scanner->line_positions_size = 5;
    scanner->line_positions_next = 1;
    scanner->line_positions[0] = scanner->source;

    return scanner;
}

/*
 * Get the next token
 *
 * This function generates the next token from the source file, including the
 * final EOF token. When finished, the function returns NULL.
 */
Token *Scanner_get_next(Scanner *scanner)
{
    TokenType token_type;
    int token_position, token_line_number;

    while (true)
    {
        token_position = scanner->current;
        token_line_number = scanner->line_number;

        if (scanner->current >= strlen(scanner->source))
        {
            token_type = END_OF_FILE;
            break;
        }
        if (match_whitespace(scanner))
        {
            continue;
        }

        token_type = get_next_token_type(scanner);
        if (token_type != NAT)
            break;
    }

    Token *token = calloc(1, sizeof(Token));
    token->type = token_type;
    token->pos.line = token_line_number;
    token->pos.position = token_position - scanner->line_start_position;

    // Make a copy of the lexeme.
    int token_len = scanner->current - token_position;
    token->lexeme = calloc(1, token_len + 1);
    strncpy(token->lexeme, scanner->source + token_position, token_len);

    // The literal value for the token, if it's a
    // string literal or constant.
    if (token->type == CONSTANT)
        token->literal.const_value = atoi(token->lexeme);

    return token;
}

/*
 * Destroy instance of ACC's Scanner.
 */
void Scanner_destroy(Scanner *scanner)
{
    // Free up scanner.
    free(scanner);
}

/*
 * Get pointer to line position in the source file
 */
const char *Scanner_get_line(Scanner *scanner, int line)
{
    if (line > scanner->line_positions_next)
        return NULL;

    return scanner->line_positions[line];
}
