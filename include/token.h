/*
 * ACC Tokens used during by the lexer/scanner
 *
 * Tokens are the 'microsyntax' of the language, as defined
 * by C's lexical grammar. The Scanner/Lexer generates a flat sequence
 * of tokens from the input source, which the parser takes as input.
 *
 * Only a subset of C11 tokens have been implemented (see TokenType_t, below).
 */

#ifndef __TOKEN__
#define __TOKEN__

/* Token types Enum */
typedef enum TokenType_t
{
    // NAT - Not a Token. Used internally in the scanner.
    NAT = 0,

    AUTO,
    CHAR,
    CONST,
    ELSE,
    EXTERN,
    IF,
    INT,
    LONG,
    REGISTER,
    RETURN,
    SHORT,
    SIGNED,
    SIZEOF,
    STATIC,
    UNSIGNED,
    VOID,
    VOLATILE,
    WHILE,

    RIGHT_ASSIGN, // '>>='
    LEFT_ASSIGN,  // '<<='
    ADD_ASSIGN,   // '+='
    SUB_ASSIGN,   // '-='
    MUL_ASSIGN,   // '*='
    DIV_ASSIGN,   // '/='
    MOD_ASSIGN,   // '%='
    AND_ASSIGN,   // '&='
    XOR_ASSIGN,   // '^='
    OR_ASSIGN,    // '|='
    RIGHT_OP,     // '>>'
    LEFT_OP,      // '<<'
    INC_OP,       // '++'
    DEC_OP,       // '--'
    PTR_OP,       // '->'
    AND_OP,       // '&&'
    OR_OP,        // '||'
    LE_OP,        // '<='
    GE_OP,        // '>='
    EQ_OP,        // '=='
    NE_OP,        // '!='

    SEMICOLON,    // ';' x
    LEFT_BRACE,   // '{' x
    RIGHT_BRACE,  // '}' x
    COMMA,        // ',' x
    COLON,        // ':' x
    EQUAL,        // '='
    LEFT_PAREN,   // '(' x
    RIGHT_PAREN,  // ')' x
    LEFT_SQUARE,  // '[' x
    RIGHT_SQUARE, // ']' x
    AMPERSAND,    // '&'
    BANG,         // '!'
    TILDE,        // '~' x
    MINUS,        // '-'
    PLUS,         // '+'
    STAR,         // '*'
    SLASH,        // '/'
    PERCENT,      // '%'
    LESS_THAN,    // '<'
    GREATER_THAN, // '>'
    CARET,        // '^'
    BAR,          // '|'
    QUESTION,     // '?'

    IDENTIFIER,
    CONSTANT,
    END_OF_FILE
} TokenType;

/* Token position */
typedef struct Position_t
{
    int line;
    int position;
} Position;

/* Token struct */
typedef struct Token_t
{
    /* Type of this token */
    TokenType type;

    Position pos;

    /* Token lexeme. Null-terminated string */
    char *lexeme;

    union {
        long int const_value;
    } literal;
} Token;

char *Token_str(TokenType);

#endif
