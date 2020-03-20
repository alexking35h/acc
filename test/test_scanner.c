#include <check.h>

#include <stdbool.h>
#include <stdio.h>
#include "scanner.h"

#define TOKEN(type, line_number, ptr, len) \
  { (type), (line_number), (ptr) }

#define COUNT(s) (sizeof(s) / sizeof(s[0]))

START_TEST(initialize_scanner) {
  Scanner* scanner = Scanner_init("");
  Scanner_destroy(scanner);
}
END_TEST

START_TEST(whitespace) {
  Scanner* scanner = Scanner_init(" \t \v \n \f  ");

  Token* eof_token = Scanner_get_next(scanner);
  ck_assert(eof_token->type == END_OF_FILE);
  ck_assert(eof_token->line_number == 2);

  Scanner_destroy(scanner);
}
END_TEST

START_TEST(single_character) {
  char* source = ";{},:=()[].&!~-+*/%<>^|?";
  Scanner* scanner = Scanner_init(source);

  TokenType expected_tokens[] = {
      SEMICOLON,    LEFT_BRACE, RIGHT_BRACE, COMMA,       COLON,
      EQUAL,        LEFT_PAREN, RIGHT_PAREN, LEFT_SQUARE, RIGHT_SQUARE,
      DOT,          AMPERSAND,  BANG,        TILDE,       MINUS,
      PLUS,         STAR,       SLASH,       PERCENT,     LESS_THAN,
      GREATER_THAN, CARET,      BAR,         QUESTION,    END_OF_FILE,
  };

  for (int i = 0; i < COUNT(expected_tokens); i++) {
    Token* token = Scanner_get_next(scanner);
    ck_assert(token->type == expected_tokens[i]);
    ck_assert(token->line_number == 1);
  }
}
END_TEST

START_TEST(assignments) {
  char* source = "|= ^= &= %= /= *= -= += <<= >>= ";
  Scanner* scanner = Scanner_init(source);

  TokenType expected_tokens[] = {
      OR_ASSIGN,   XOR_ASSIGN,   AND_ASSIGN,  MOD_ASSIGN,
      DIV_ASSIGN,  MUL_ASSIGN,   SUB_ASSIGN,  ADD_ASSIGN,
      LEFT_ASSIGN, RIGHT_ASSIGN, END_OF_FILE,
  };

  for (int i = 0; i < COUNT(expected_tokens); i++) {
    Token* token = Scanner_get_next(scanner);
    ck_assert(token->type == expected_tokens[i]);
    ck_assert(token->line_number == 1);
  }
}
END_TEST

START_TEST(comment) {
  char* source = ":   // jim\n;/* pam\n\n */\n!";
  Scanner* scanner = Scanner_init(source);

  Token* token = Scanner_get_next(scanner);
  ck_assert(token->type = COLON);
  ck_assert(token->line_number == 1);

  token = Scanner_get_next(scanner);
  ck_assert(token->type == SEMICOLON);
  ck_assert(token->line_number == 2);

  token = Scanner_get_next(scanner);
  ck_assert(token->type == BANG);
  ck_assert(token->line_number == 5);
}
END_TEST

START_TEST(string_literal) {
  char* source = " \"qwertyuiop\" \"qwert\\\"yuiop\\\t\"";
  Scanner* scanner = Scanner_init(source);

  Token* string_token = Scanner_get_next(scanner);
  ck_assert(string_token->type == STRING_LITERAL);
  ck_assert(string_token->line_number == 1);
  ck_assert(strcmp(string_token->lexeme, "\"qwertyuiop\"") == 0);

  string_token = Scanner_get_next(scanner);
  ck_assert(string_token->type == STRING_LITERAL);
  ck_assert(string_token->line_number == 1);
  ck_assert(strcmp(string_token->lexeme, "\"qwert\\\"yuiop\\\t\"") == 0);
}
END_TEST

START_TEST(hex_literal) {
  const char* source =
      "0x123abc  0X123abc  0x123abcU 0X123abcu 0x123abcl 0x123abcL";
  Scanner* scanner = Scanner_init(source);

  int expected_lexeme_length[] = {8, 8, 9, 9, 9, 9};
  for (int i = 0; i < COUNT(expected_lexeme_length); i++) {
    Token* number_token = Scanner_get_next(scanner);
    ck_assert(number_token->type == CONSTANT);
    ck_assert(number_token->line_number == 1);
    ck_assert(strncmp(number_token->lexeme, source + (i * 10),
                      expected_lexeme_length[i]) == 0);
  }
}
END_TEST

START_TEST(keyword) {
  const char* source =
      "auto break case char const continue default do double else "
      "enum extern float for goto if inline int long register "
      "restrict return short signed sizeof static struct switch "
      "typedef union unsigned void volatile while _identifier_1234_name";

  TokenType keyword_tokens[] = {
      AUTO,    BREAK,  CASE,     CHAR,   CONST,    CONTINUE, DEFAULT,
      DO,      DOUBLE, ELSE,     ENUM,   EXTERN,   FLOAT,    FOR,
      GOTO,    IF,     INLINE,   INT,    LONG,     REGISTER, RESTRICT,
      RETURN,  SHORT,  SIGNED,   SIZEOF, STATIC,   STRUCT,   SWITCH,
      TYPEDEF, UNION,  UNSIGNED, VOID,   VOLATILE, WHILE,    IDENTIFIER,
      END_OF_FILE};

  Scanner* scanner = Scanner_init(source);
  for (int i = 0; i < COUNT(keyword_tokens); i++) {
    Token* token = Scanner_get_next(scanner);
    ck_assert(token->type == keyword_tokens[i]);
  }
}
END_TEST

TCase* scanner_testcase(void) {
  TCase* testcase = tcase_create("scanner");

  tcase_add_test(testcase, initialize_scanner);
  tcase_add_test(testcase, whitespace);
  tcase_add_test(testcase, single_character);
  tcase_add_test(testcase, assignments);
  tcase_add_test(testcase, comment);
  tcase_add_test(testcase, string_literal);
  tcase_add_test(testcase, hex_literal);
  tcase_add_test(testcase, keyword);
  return testcase;
}
