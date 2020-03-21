#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "error.h"
#include "scanner.h"
#include "token.h"

#define TOKEN_BUFFER_SIZE 32
#define IDENTIFIER_START_CHARACTERS \
  "QWERTYUIOPASDFGHJKLZXCVBNMqwertyuiopasdfghjklzxcvbnm_"
#define IDENTIFIER_CHARACTERS \
  "QWERTYUIOPASDFGHJKLZXCVBNMqwertyuiopasdfghjklzxcvbnm_1234567890"

#define MAX_KEYWORD_LENGTH 20

#define count(n) sizeof(n) / sizeof(n[0])

/*
 * Tokens are allocated in blocks of TOKEN_BUFFER_SIZE,
 * and stored in a linked list so that new tokens can be
 * created on-demand.
 */
typedef struct TokenList_t {
  // Linked-list pointer
  struct TokenList_t *next;

  Token tokens[TOKEN_BUFFER_SIZE];
} TokenList;

/*
 * Scanner class
 * This struct encapsulates all state required by the scanner.
 */
typedef struct Scanner_t {
  // Source file
  const char *source;

  // Error handler instance.
  Error *error_handler;

  // Current position in the input.
  int current;
  int line_number;

  // Allocated tokens
  TokenList *token_list;
  Token *next_token;
  int token_count;
} Scanner;

static Token *get_token(Scanner *);

/*
 * Helper functions:
 * match_*   - return true if the input matches, advance the scanner position
 * consume_* - advance the scanner over the input, raise error if the input
 * doesn't match
 */
static bool match_character(Scanner *, const char *expected);
static bool match_whitespace(Scanner *);
static void consume_keyword_or_identifier(Scanner *, TokenType *);
static void consume_number(Scanner *);
static bool consume_string(Scanner *);
static void consume_comment(Scanner *);

static char peek(Scanner *);
static bool end_of_file(Scanner *);

static Token *get_token(Scanner *scanner) {
  if ((++scanner->token_count == TOKEN_BUFFER_SIZE) != 0) {
    return scanner->next_token++;
  } else {
    // Allocate more space for tokens.
    TokenList *token_list = scanner->token_list;
    while (token_list->next) {
      token_list = token_list->next;
    }
    token_list->next = (TokenList *)calloc(1, sizeof(TokenList));
    scanner->next_token = token_list->next->tokens;
    scanner->token_count = 0;

    return scanner->next_token;
  }
}

static char peek(Scanner *scanner) {
  if (scanner->current >= strlen(scanner->source)) return 0;

  return scanner->source[scanner->current];
}

static bool end_of_file(Scanner *scanner) {
  if (scanner->current >= strlen(scanner->source)) return true;
  return false;
}

static bool match_character(Scanner *scanner, const char *expected) {
  for (; *expected; expected++) {
    if (scanner->source[scanner->current] == *expected) {
      scanner->current++;
      return true;
    }
  }
  return false;
}

static bool match_whitespace(Scanner *scanner) {
  char whitespace[] = {'\t', '\v', '\f', ' '};
  char focus = scanner->source[scanner->current];
  if (focus == '\n') {
    scanner->line_number++;
    scanner->current++;
    return true;
  }
  for (int i = 0; i < count(whitespace); i++) {
    if (focus == whitespace[i]) {
      scanner->current++;
      return true;
    }
  }
  return false;
}

static void consume_keyword_or_identifier(Scanner *scanner,
                                          TokenType *token_type) {
  const char *identifier_start = scanner->source + scanner->current;
  int identifier_length = 1;

  scanner->current++;

  while (match_character(scanner, IDENTIFIER_CHARACTERS)) {
    identifier_length++;
  }

  char *keywords[] = {
      "auto",     "break",    "case",     "char",   "const",   "continue",
      "default",  "do",       "double",   "else",   "enum",    "extern",
      "float",    "for",      "goto",     "if",     "inline",  "int",
      "long",     "register", "restrict", "return", "short",   "signed",
      "sizeof",   "static",   "struct",   "switch", "typedef", "union",
      "unsigned", "void",     "volatile", "while",
  };

  for (int i = 0; i < count(keywords); i++) {
    if (strlen(keywords[i]) != identifier_length) continue;
    if (strncmp(identifier_start, keywords[i], identifier_length)) continue;

    *token_type = AUTO + i;
    return;
  }
  *token_type = IDENTIFIER;
}

static bool consume_string(Scanner *scanner) {
  // Skip over the first double-quote
  int string_line_number = scanner->line_number;
  scanner->current++;
  while (true) {
    if (end_of_file(scanner)) {
      Error_report_error(scanner->error_handler, SCANNER, scanner->line_number,
                         "Unterminated string literal");
      return false;
    }

    char focus = scanner->source[scanner->current++];

    // End of string, stop searching.
    if (focus == '"') break;

    // Reach end of line (before end of string).
    if (focus == '\n') {
      scanner->line_number++;
      Error_report_error(scanner->error_handler, SCANNER, string_line_number,
                         "Unterminated string literal");
      return false;
    }

    // Escape sequence.
    // These are replaced later. But make sure to ignore the double-quote.
    if (focus == '\\') {
      if (match_character(scanner, "\"")) continue;
    }
  }
  return true;
}

/* Consume a number */
static void consume_number(Scanner *scanner) {
  if (match_character(scanner, "0")) {
    if (match_character(scanner, "Xx")) {
      // Hex notation
      while (match_character(scanner, "1234567890abcdefABCDEF")) continue;
    } else {
      // Oct notation
      while (match_character(scanner, "1234567890")) continue;
    }
  } else {
    while (match_character(scanner, "1234567890")) continue;
  }
  match_character(scanner, "ulUL");
}

/* Consume a comment */
static void consume_comment(Scanner *scanner) {
  scanner->current++;
  if (match_character(scanner, "/")) {
    while (scanner->source[scanner->current] != '\n') scanner->current++;
  } else if (match_character(scanner, "*")) {
    while (true) {
      if (match_character(scanner, "\n")) {
        scanner->line_number++;
        continue;
      }
      if (scanner->source[scanner->current++] != '*') continue;
      if (scanner->source[scanner->current++] != '/') continue;
      break;
    }
  }
}

static TokenType get_next_token_type(Scanner *scanner) {
  char focus = scanner->source[scanner->current++];

  // Single-character tokens
  switch (focus) {
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
  if (focus == '=') return match_character(scanner, "=") ? EQ_OP : EQUAL;

  if (focus == '!') return match_character(scanner, "=") ? NE_OP : BANG;

  // Assignments
  if (focus == '-') return match_character(scanner, "=") ? SUB_ASSIGN : MINUS;

  if (focus == '+') return match_character(scanner, "=") ? ADD_ASSIGN : PLUS;

  if (focus == '*') return match_character(scanner, "=") ? MUL_ASSIGN : STAR;

  if (focus == '%') return match_character(scanner, "=") ? MOD_ASSIGN : PERCENT;

  if (focus == '^') return match_character(scanner, "=") ? XOR_ASSIGN : CARET;

  if (focus == '|') return match_character(scanner, "=") ? OR_ASSIGN : BAR;

  if (focus == '&') {
    if (match_character(scanner, "="))
      return AND_ASSIGN;

    else if (match_character(scanner, "&"))
      return AND_OP;

    else
      return AMPERSAND;
  }

  if (focus == '<') {
    if (match_character(scanner, "<")) {
      return match_character(scanner, "=") ? LEFT_ASSIGN : LEFT_OP;
    }
    if (match_character(scanner, "=")) return LE_OP;

    return LESS_THAN;
  }
  if (focus == '>') {
    if (match_character(scanner, ">"))
      return match_character(scanner, "=") ? RIGHT_ASSIGN : RIGHT_OP;

    if (match_character(scanner, "=")) return GE_OP;

    return GREATER_THAN;
  }

  if (focus == '/') {
    // Comment.
    if (peek(scanner) == '*') {
      scanner->current--;
      consume_comment(scanner);
      return 0;
    } else if (peek(scanner) == '/') {
      scanner->current--;
      consume_comment(scanner);
      return 0;
    }

    return match_character(scanner, "=") ? DIV_ASSIGN : SLASH;
  }

  // String literal.
  if (focus == '"') {
    scanner->current--;
    return consume_string(scanner) ? STRING_LITERAL : 0;
  }

  // Number literal.
  if (strchr("1234567890", focus)) {
    scanner->current--;
    consume_number(scanner);
    return CONSTANT;
  }

  if (strchr(IDENTIFIER_START_CHARACTERS, focus)) {
    scanner->current--;
    TokenType token;
    consume_keyword_or_identifier(scanner, &token);
    return token;
  }

  char error_string[50];
  snprintf(error_string, 50, "Invalid character in input: '%c'", focus);
  Error_report_error(scanner->error_handler, SCANNER, scanner->line_number,
                     error_string);

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
Scanner *Scanner_init(char const *source, Error *error_handler) {
  Scanner *scanner = malloc(sizeof(Scanner));

  scanner->error_handler = error_handler;
  scanner->source = source;
  scanner->current = 0;
  scanner->line_number = 1;

  // Allocate the first block of tokens.
  scanner->token_list = (TokenList *)calloc(1, sizeof(TokenList));
  scanner->token_count = 0;
  scanner->next_token = scanner->token_list->tokens;

  return scanner;
}

/*
 * Get the next token
 *
 * This function generates the next token from the source file, including the
 * final EOF token. When finished, the function returns NULL.
 */
Token *Scanner_get_next(Scanner *scanner) {
  TokenType token_type;
  int token_position, token_line_number;

  while (true) {
    token_position = scanner->current;
    token_line_number = scanner->line_number;

    if (scanner->current >= strlen(scanner->source)) {
      token_type = END_OF_FILE;
      break;
    }
    if (match_whitespace(scanner)) {
      continue;
    }
    if ((token_type = get_next_token_type(scanner))) {
      break;
    }
  }

  Token *token = get_token(scanner);
  token->type = token_type;
  token->line_number = token_line_number;

  if (TOKEN_STORE_LEXEME(token->type)) {
    const char *src = &(scanner->source[token_position]);
    int len = scanner->current - token_position;
    token->lexeme = calloc(1, len + 1);
    strncpy(token->lexeme, src, len);
  } else {
    token->lexeme = NULL;
  }
  return token;
}

/*
 * Destroy instance of ACC's Scanner.
 */
void Scanner_destroy(Scanner *scanner) {
  // Free up tokens.
  TokenList *token_list = scanner->token_list;
  while (token_list) {
    TokenList *p = token_list->next;
    free(token_list);
    token_list = p;
  }

  // Free up scanner.
  free(scanner);
}
