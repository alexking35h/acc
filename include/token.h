#ifndef __TOKEN__
#define __TOKEN__

#define TOKEN_STORE_LEXEME(tok) (tok >= 100)

// ALIGNAS
// ALIGNOF
// ATOMIC
// BOOL
// COMPLEX
// GENERIC
// IMAGINARY
// NORETURN
// STATIC_ASSERT
// THREAD_LOCAL
// FUNC_NAME

/* Token types Enum */
typedef enum TokenType_t {
  AUTO = 1,
  BREAK,
  CASE,
  CHAR,
  CONST,
  CONTINUE,
  DEFAULT,
  DO,
  DOUBLE,
  ELSE,
  ENUM,
  EXTERN,
  FLOAT,
  FOR,
  GOTO,
  IF,
  INLINE,
  INT,
  LONG,
  REGISTER,
  RESTRICT,
  RETURN,
  SHORT,
  SIGNED,
  SIZEOF,
  STATIC,
  STRUCT,
  SWITCH,
  TYPEDEF,
  UNION,
  UNSIGNED,
  VOID,
  VOLATILE,
  WHILE,

  ELLIPSIS,      // '...'
  RIGHT_ASSIGN,  // '>>='
  LEFT_ASSIGN,   // '<<='
  ADD_ASSIGN,    // '+='
  SUB_ASSIGN,    // '-='
  MUL_ASSIGN,    // '*='
  DIV_ASSIGN,    // '/='
  MOD_ASSIGN,    // '%='
  AND_ASSIGN,    // '&='
  XOR_ASSIGN,    // '^='
  OR_ASSIGN,     // '|='
  RIGHT_OP,      // '>>'
  LEFT_OP,       // '<<'
  INC_OP,        // '++'
  DEC_OP,        // '--'
  PTR_OP,        // '->'
  AND_OP,        // '&&'
  OR_OP,         // '||'
  LE_OP,         // '<='
  GE_OP,         // '>='
  EQ_OP,         // '=='
  NE_OP,         // '!='

  SEMICOLON,     // ';' x
  LEFT_BRACE,    // '{' x
  RIGHT_BRACE,   // '}' x
  COMMA,         // ',' x
  COLON,         // ':' x
  EQUAL,         // '='
  LEFT_PAREN,    // '(' x
  RIGHT_PAREN,   // ')' x
  LEFT_SQUARE,   // '[' x
  RIGHT_SQUARE,  // ']' x
  DOT,           // '.'
  AMPERSAND,     // '&'
  BANG,          // '!'
  TILDE,         // '~' x
  MINUS,         // '-'
  PLUS,          // '+'
  STAR,          // '*'
  SLASH,         // '/'
  PERCENT,       // '%'
  LESS_THAN,     // '<'
  GREATER_THAN,  // '>'
  CARET,         // '^'
  BAR,           // '|'
  QUESTION,      // '?'

  END_OF_FILE,

  IDENTIFIER = 100,

  CONSTANT,
  STRING_LITERAL
} TokenType;

/* Token struct */
typedef struct Token_t {
  /* Type of this token */
  TokenType type;

  /* Position of the Token in the input */
  int line_number;

  /* Token lexeme. Null-terminated string */
  char* lexeme;
} Token;

#endif
