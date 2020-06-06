#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <cmocka.h>


#include "error.h"
#include "scanner.h"

#define TOKEN(type, line_number, ptr, len) \
  { (type), (line_number), (ptr) }

#define COUNT(s) (sizeof(s) / sizeof(s[0]))

#define MOCK_ERROR_REPORTER (ErrorReporter*)0x1234

void __wrap_Error_report_error(
  ErrorReporter* error_reporter,
  ErrorType error_type,
  int line_number,
  int line_position,
  const char* title,
  const char* description) 
{
  function_called();
  assert_true(error_reporter == MOCK_ERROR_REPORTER);
  assert_true(error_type == SCANNER);
  check_expected(line_number);
  check_expected(line_position);
  check_expected(title);
}

static void initialize_scanner(void** state) {
  Scanner* scanner = Scanner_init("", MOCK_ERROR_REPORTER);
  Scanner_destroy(scanner);
}

static void whitespace(void** state) {
  Scanner* scanner = Scanner_init(" \t \v \n \f  ", MOCK_ERROR_REPORTER);

  Token* eof_token = Scanner_get_next(scanner);
  assert_int_equal(eof_token->type, END_OF_FILE);
  assert_int_equal(eof_token->line_number, 2);
  assert_int_equal(eof_token->line_position, 4);

  Scanner_destroy(scanner);
}

static void single_character(void** state) {
  char* source = ";{},:=()[].&!~-+*/%<>^|?";
  Scanner* scanner = Scanner_init(source, MOCK_ERROR_REPORTER);

  TokenType expected_tokens[] = {
      SEMICOLON,    LEFT_BRACE, RIGHT_BRACE, COMMA,       COLON,
      EQUAL,        LEFT_PAREN, RIGHT_PAREN, LEFT_SQUARE, RIGHT_SQUARE,
      DOT,          AMPERSAND,  BANG,        TILDE,       MINUS,
      PLUS,         STAR,       SLASH,       PERCENT,     LESS_THAN,
      GREATER_THAN, CARET,      BAR,         QUESTION,    END_OF_FILE,
  };

  for (int i = 0; i < COUNT(expected_tokens); i++) {
    Token* token = Scanner_get_next(scanner);
    assert_int_equal(token->type, expected_tokens[i]);
    assert_int_equal(token->line_number, 1);
    assert_int_equal(token->line_position, i);
  }
}

static void assignments(void** state) {
  char* source = "|= ^= &= %= /= *= -= += <<= >>= ";
  Scanner* scanner = Scanner_init(source, MOCK_ERROR_REPORTER);

  TokenType expected_tokens[] = {
      OR_ASSIGN,   XOR_ASSIGN,   AND_ASSIGN,  MOD_ASSIGN,
      DIV_ASSIGN,  MUL_ASSIGN,   SUB_ASSIGN,  ADD_ASSIGN,
      LEFT_ASSIGN, RIGHT_ASSIGN, END_OF_FILE,
  };
  int expected_positions[] = {0, 3, 6, 9, 12, 15, 18, 21, 24, 28, 32};

  for (int i = 0; i < COUNT(expected_tokens); i++) {
    Token* token = Scanner_get_next(scanner);
    assert_int_equal(token->type, expected_tokens[i]);
    assert_int_equal(token->line_position, expected_positions[i]);
    assert_int_equal(token->line_number, 1);
  }
}

static void operators(void** state) {
  Scanner* scanner = Scanner_init(">> << ++ -- -> && || <= >= == !=", MOCK_ERROR_REPORTER);

  TokenType expected_tokens[] = {RIGHT_OP, LEFT_OP, INC_OP, DEC_OP,
                                 PTR_OP,   AND_OP,  OR_OP,  LE_OP,
                                 GE_OP,    EQ_OP,   NE_OP};

  for (int i = 0; i < COUNT(expected_tokens); i++) {
    Token* token = Scanner_get_next(scanner);
    assert_true(expected_tokens[i] == token->type);
    assert_int_equal(token->line_number, 1);
    assert_int_equal(token->line_position, i*3);
  }
}

static void comment(void** state) {
  char* source = ":   // jim\n;/* pam\n\n */\n!";
  Scanner* scanner = Scanner_init(source, MOCK_ERROR_REPORTER);

  Token* token = Scanner_get_next(scanner);
  assert_int_equal(token->type, COLON);
  assert_int_equal(token->line_number, 1);
  assert_int_equal(token->line_position, 0);

  token = Scanner_get_next(scanner);
  assert_int_equal(token->type, SEMICOLON);
  assert_int_equal(token->line_number, 2);
  assert_int_equal(token->line_position, 0);

  token = Scanner_get_next(scanner);
  assert_int_equal(token->type, BANG);
  assert_int_equal(token->line_number, 5);
  assert_int_equal(token->line_position, 0);
}

static void string_literal(void** state) {
  char* source = " \"qwertyuiop\" \"qwert\\\"yuiop\\\t\"";
  Scanner* scanner = Scanner_init(source, MOCK_ERROR_REPORTER);

  Token* string_token = Scanner_get_next(scanner);
  assert_int_equal(string_token->type, STRING_LITERAL);
  assert_int_equal(string_token->line_number, 1);
  assert_int_equal(string_token->line_position, 1);
  assert_string_equal(string_token->lexeme, "\"qwertyuiop\"");

  string_token = Scanner_get_next(scanner);
  assert_int_equal(string_token->type, STRING_LITERAL);
  assert_int_equal(string_token->line_number, 1);
  assert_int_equal(string_token->line_position, 14);
  assert_string_equal(string_token->lexeme, "\"qwert\\\"yuiop\\\t\"");
}

static void hex_literal(void** state) {
  const char* source =
      "0x123abc 0X123abc 0x123abU 0X123abu 0x123abl 0x123abL";
  Scanner* scanner = Scanner_init(source, MOCK_ERROR_REPORTER);

  int expected_lexeme_length[] = {8, 8, 9, 9, 9, 9};
  for (int i = 0; i < COUNT(expected_lexeme_length); i++) {
    Token* number_token = Scanner_get_next(scanner);
    assert_int_equal(number_token->type, CONSTANT);
    assert_int_equal(number_token->line_number, 1);
    assert_int_equal(number_token->line_position, i*9);
    assert_true(strncmp(number_token->lexeme, source + (i * 9), 8) == 0);
  }
}

static void keyword(void** state) {
  const char* source =
      "auto break case char const continue default do double else "
      "enum extern float for goto if inline int long register "
      "restrict return short signed sizeof static struct switch "
      "typedef union unsigned void volatile while _identifier_1234_name";

  TokenType keyword_tokens[] = {
      AUTO,     BREAK,    CASE,     CHAR,   CONST,      CONTINUE,
      DEFAULT,  DO,       DOUBLE,   ELSE,   ENUM,       EXTERN,
      FLOAT,    FOR,      GOTO,     IF,     INLINE,     INT,
      LONG,     REGISTER, RESTRICT, RETURN, SHORT,      SIGNED,
      SIZEOF,   STATIC,   STRUCT,   SWITCH, TYPEDEF,    UNION,
      UNSIGNED, VOID,     VOLATILE, WHILE,  IDENTIFIER, END_OF_FILE};

  Scanner* scanner = Scanner_init(source, MOCK_ERROR_REPORTER);
  for (int i = 0; i < COUNT(keyword_tokens); i++) {
    Token* token = Scanner_get_next(scanner);
    assert_true(token->type == keyword_tokens[i]);
    assert_int_equal(token->type, keyword_tokens[i]);
    assert_int_equal(token->line_number, 1);
  }
}

static void invalid_character(void** state) {
  const char* source = " 432\n @ ";
  Scanner* scanner = Scanner_init(source, MOCK_ERROR_REPORTER);

  Token* token = Scanner_get_next(scanner);
  assert_int_equal(token->type, CONSTANT);

  expect_function_call(__wrap_Error_report_error);
  expect_value(__wrap_Error_report_error, line_number, 2);
  expect_value(__wrap_Error_report_error, line_position, 1);
  expect_string(__wrap_Error_report_error, title, "Invalid character in input: '@'");

  token = Scanner_get_next(scanner);
  assert_int_equal(token->type, END_OF_FILE);
}

static void unterminated_string(void** state) {
  const char* source = " s = \"test;\nb = \"";
  Scanner* scanner = Scanner_init(source, MOCK_ERROR_REPORTER);

  Token* token = Scanner_get_next(scanner);
  assert_int_equal(token->type, IDENTIFIER);

  token = Scanner_get_next(scanner);
  assert_int_equal(token->type, EQUAL);

  expect_function_call(__wrap_Error_report_error);
  expect_value(__wrap_Error_report_error, line_number, 1);
  expect_value(__wrap_Error_report_error, line_position, 5);
  expect_string(__wrap_Error_report_error, title,
                "Unterminated string literal");

  token = Scanner_get_next(scanner);
  assert_int_equal(token->type, IDENTIFIER);
  token = Scanner_get_next(scanner);
  assert_int_equal(token->type, EQUAL);

  expect_function_call(__wrap_Error_report_error);
  expect_value(__wrap_Error_report_error, line_number, 2);
  expect_value(__wrap_Error_report_error, line_position, 4);
  expect_string(__wrap_Error_report_error, title,
                "Unterminated string literal");

  token = Scanner_get_next(scanner);
  assert_int_equal(token->type, END_OF_FILE);
}

int main(void) {
  const struct CMUnitTest tests[] = {cmocka_unit_test(initialize_scanner),
                                     cmocka_unit_test(whitespace),
                                     cmocka_unit_test(single_character),
                                     cmocka_unit_test(assignments),
                                     cmocka_unit_test(operators),
                                     cmocka_unit_test(comment),
                                     cmocka_unit_test(string_literal),
                                     cmocka_unit_test(hex_literal),
                                     cmocka_unit_test(keyword),
                                     cmocka_unit_test(invalid_character),
                                     cmocka_unit_test(unterminated_string)};
  return cmocka_run_group_tests(tests, NULL, NULL);
}
