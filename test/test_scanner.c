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

#define TOKEN(type, line_number, ptr, len)                                               \
    {                                                                                    \
        (type), (line_number), (ptr)                                                     \
    }

#define COUNT(s) (sizeof(s) / sizeof(s[0]))

#define MOCK_ERROR_REPORTER (ErrorReporter *)0x1234

void __wrap_Error_report_error(ErrorReporter *error_reporter, ErrorType error_type,
                               Position pos, const char *title, const char *description)
{
    int line_number = pos.line;
    int line_position = pos.position;
    function_called();
    assert_true(error_reporter == MOCK_ERROR_REPORTER);
    assert_true(error_type == SCANNER);
    check_expected(line_number);
    check_expected(line_position);
    check_expected(title);
}

static void initialize_scanner(void **state)
{
    Scanner *scanner = Scanner_init("", MOCK_ERROR_REPORTER);
    Scanner_destroy(scanner);
}

static void whitespace(void **state)
{
    Scanner *scanner = Scanner_init(" \t \n    ", MOCK_ERROR_REPORTER);

    Token *eof_token = Scanner_get_next(scanner);
    assert_int_equal(eof_token->type, END_OF_FILE);
    assert_int_equal(eof_token->pos.line, 2);
    assert_int_equal(eof_token->pos.position, 4);

    Scanner_destroy(scanner);
}

static void single_character(void **state)
{
    char *source = ";{},:=()[]&!~-+*/%<>^|?";
    Scanner *scanner = Scanner_init(source, MOCK_ERROR_REPORTER);

    TokenType expected_tokens[] = {
        SEMICOLON,    LEFT_BRACE, RIGHT_BRACE, COMMA,       COLON,
        EQUAL,        LEFT_PAREN, RIGHT_PAREN, LEFT_SQUARE, RIGHT_SQUARE,
                  AMPERSAND,  BANG,        TILDE,       MINUS,
        PLUS,         STAR,       SLASH,       PERCENT,     LESS_THAN,
        GREATER_THAN, CARET,      BAR,         QUESTION,    END_OF_FILE,
    };

    for (int i = 0; i < COUNT(expected_tokens); i++)
    {
        Token *token = Scanner_get_next(scanner);
        assert_int_equal(token->type, expected_tokens[i]);
        assert_int_equal(token->pos.line, 1);
        assert_int_equal(token->pos.position, i);
    }
}

static void assignments(void **state)
{
    char *source = "|= ^= &= %= /= *= -= += <<= >>= ";
    Scanner *scanner = Scanner_init(source, MOCK_ERROR_REPORTER);

    TokenType expected_tokens[] = {
        OR_ASSIGN,  XOR_ASSIGN, AND_ASSIGN,  MOD_ASSIGN,   DIV_ASSIGN,  MUL_ASSIGN,
        SUB_ASSIGN, ADD_ASSIGN, LEFT_ASSIGN, RIGHT_ASSIGN, END_OF_FILE,
    };
    int expected_positions[] = {0, 3, 6, 9, 12, 15, 18, 21, 24, 28, 32};

    for (int i = 0; i < COUNT(expected_tokens); i++)
    {
        Token *token = Scanner_get_next(scanner);
        assert_int_equal(token->type, expected_tokens[i]);
        assert_int_equal(token->pos.position, expected_positions[i]);
        assert_int_equal(token->pos.line, 1);
    }
}

static void operators(void **state)
{
    Scanner *scanner =
        Scanner_init(">> << ++ -- -> && || <= >= == !=", MOCK_ERROR_REPORTER);

    TokenType expected_tokens[] = {RIGHT_OP, LEFT_OP, INC_OP, DEC_OP, PTR_OP, AND_OP,
                                   OR_OP,    LE_OP,   GE_OP,  EQ_OP,  NE_OP};

    for (int i = 0; i < COUNT(expected_tokens); i++)
    {
        Token *token = Scanner_get_next(scanner);
        assert_true(expected_tokens[i] == token->type);
        assert_int_equal(token->pos.line, 1);
        assert_int_equal(token->pos.position, i * 3);
    }
}

static void comment(void **state)
{
    char *source = ":   // jim\n;/* pam\n\n */\n!";
    Scanner *scanner = Scanner_init(source, MOCK_ERROR_REPORTER);

    Token *token = Scanner_get_next(scanner);
    assert_int_equal(token->type, COLON);
    assert_int_equal(token->pos.line, 1);
    assert_int_equal(token->pos.position, 0);

    token = Scanner_get_next(scanner);
    assert_int_equal(token->type, SEMICOLON);
    assert_int_equal(token->pos.line, 2);
    assert_int_equal(token->pos.position, 0);

    token = Scanner_get_next(scanner);
    assert_int_equal(token->type, BANG);
    assert_int_equal(token->pos.line, 5);
    assert_int_equal(token->pos.position, 0);
}

static void keyword(void **state)
{
    const char *source =
        "auto char const else "
        "extern if int long register "
        "return short signed sizeof static "
        "unsigned void volatile while _identifier_1234_name";

    TokenType keyword_tokens[] = {
        AUTO,          CHAR,       CONST,    
        ELSE,          EXTERN,                IF,
           INT,    LONG,       REGISTER,    RETURN,   SHORT,    SIGNED,
        SIZEOF,   STATIC,                 UNSIGNED, VOID,
        VOLATILE, WHILE,  IDENTIFIER, END_OF_FILE};

    Scanner *scanner = Scanner_init(source, MOCK_ERROR_REPORTER);
    for (int i = 0; i < COUNT(keyword_tokens); i++)
    {
        printf("\n%u", keyword_tokens[i]);
        Token *token = Scanner_get_next(scanner);
        assert_int_equal(token->type, keyword_tokens[i]);
        assert_int_equal(token->pos.line, 1);
    }
}

static void invalid_character(void **state)
{
    const char *source = " 432\n @ ";
    Scanner *scanner = Scanner_init(source, MOCK_ERROR_REPORTER);

    Token *token = Scanner_get_next(scanner);
    assert_int_equal(token->type, CONSTANT);

    expect_function_call(__wrap_Error_report_error);
    expect_value(__wrap_Error_report_error, line_number, 2);
    expect_value(__wrap_Error_report_error, line_position, 1);
    expect_string(__wrap_Error_report_error, title, "Invalid character in input: '@'");

    token = Scanner_get_next(scanner);
    assert_int_equal(token->type, END_OF_FILE);
}

int main(void)
{
    const struct CMUnitTest tests[] = {cmocka_unit_test(initialize_scanner),
                                       cmocka_unit_test(whitespace),
                                       cmocka_unit_test(single_character),
                                       cmocka_unit_test(assignments),
                                       cmocka_unit_test(operators),
                                       cmocka_unit_test(comment),
                                       cmocka_unit_test(keyword),
                                       cmocka_unit_test(invalid_character)};
    return cmocka_run_group_tests(tests, NULL, NULL);
}
