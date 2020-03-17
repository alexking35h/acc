#include <check.h>

#include "scanner.h"
#include <stdbool.h>
#include <stdio.h>

#define TOKEN(type, line_number, ptr, len) \
    {(type), (line_number), (ptr), (len)}

#define TOKEN_COMPARE(tok_a, tok_b) \
    (tok_a->type == tok_b->type \
     && tok_a->line_number == tok_b->line_number \
     && tok_a->lexeme == tok_b->lexeme \
     && tok_a->lexeme_length == tok_b->lexeme_length)

#define COUNT(s) (sizeof(s)/sizeof(s[0]))

START_TEST(initialize_scanner)
{
    Scanner * scanner = Scanner_init("");
    Scanner_destroy(scanner);
}
END_TEST

START_TEST(whitespace)
{
    Scanner * scanner = Scanner_init(" \t \v \n \f  ");
    
    Token * eof_token = Scanner_get_next(scanner);
    ck_assert(eof_token->type == END_OF_FILE);
    ck_assert(eof_token->line_number == 2);

    Scanner_destroy(scanner);
}
END_TEST

START_TEST(single_character)
{
    char * source = ";{},:=()[].&!~-+*/%<>^|?";
    Scanner * scanner = Scanner_init(source);

    Token expected_tokens[] = {
        TOKEN(SEMICOLON, 1, source++, 1),
        TOKEN(LEFT_BRACE, 1, source++, 1),
        TOKEN(RIGHT_BRACE, 1, source++, 1),
        TOKEN(COMMA, 1, source++, 1),
        TOKEN(COLON, 1, source++, 1),
        TOKEN(EQUAL, 1, source++, 1),
        TOKEN(LEFT_PAREN, 1, source++, 1),
        TOKEN(RIGHT_PAREN, 1, source++, 1),
        TOKEN(LEFT_SQUARE, 1, source++, 1),
        TOKEN(RIGHT_SQUARE, 1, source++, 1),
        TOKEN(DOT, 1, source++, 1),
        TOKEN(AMPERSAND, 1, source++, 1),
        TOKEN(BANG, 1, source++, 1),
        TOKEN(TILDE, 1, source++, 1),
        TOKEN(MINUS, 1, source++, 1),
        TOKEN(PLUS, 1, source++, 1),
        TOKEN(STAR, 1, source++, 1),
        TOKEN(SLASH, 1, source++, 1),
        TOKEN(PERCENT, 1, source++, 1),
        TOKEN(LESS_THAN, 1, source++, 1),
        TOKEN(GREATER_THAN, 1, source++, 1),
        TOKEN(CARET, 1, source++, 1),
        TOKEN(BAR, 1, source++, 1),
        TOKEN(QUESTION, 1, source++, 1),
        TOKEN(END_OF_FILE, 1, source, 0)
    };

    for(int i = 0;i<COUNT(expected_tokens);i++)
    {
        Token * token = Scanner_get_next(scanner);
        Token * ref = &expected_tokens[i];

        ck_assert(TOKEN_COMPARE(token, ref));
    }
}
END_TEST

START_TEST(assignments)
{
    char * source = "|= ^= &= %= /= *= -= += <<= >>= ";
    Scanner * scanner = Scanner_init(source);

    Token expected_tokens[] = {
        TOKEN(OR_ASSIGN, 1, source, 2),
        TOKEN(XOR_ASSIGN, 1, source+=3, 2),
        TOKEN(AND_ASSIGN, 1, source+=3, 2),
        TOKEN(MOD_ASSIGN, 1, source+=3, 2),
        TOKEN(DIV_ASSIGN, 1, source+=3, 2),
        TOKEN(MUL_ASSIGN, 1, source+=3, 2),
        TOKEN(SUB_ASSIGN, 1, source+=3, 2),
        TOKEN(ADD_ASSIGN, 1, source+=3, 2),
        TOKEN(LEFT_ASSIGN, 1, source+=3, 3),
        TOKEN(RIGHT_ASSIGN, 1, source+=4, 3),
        TOKEN(END_OF_FILE, 1, source+=4, 0)
    };

    for(int i = 0;i<COUNT(expected_tokens);i++)
    {
        Token * token = Scanner_get_next(scanner);
        Token * ref = &expected_tokens[i];

        ck_assert(TOKEN_COMPARE(token, ref));
    }
}
END_TEST

START_TEST(string_literal)
{
    char * source = " \"qwertyuiop\" \"qwert\\\"yuiop\\\t\"";
    Scanner * scanner = Scanner_init(source);

    Token * string_token = Scanner_get_next(scanner);
    ck_assert(string_token->type == STRING_LITERAL);
    ck_assert(string_token->line_number == 1);
    ck_assert(string_token->lexeme == &(source[1]));
    ck_assert(string_token->lexeme_length == strlen("\"qwertyuiop\""));

    string_token = Scanner_get_next(scanner);
    ck_assert(string_token->type == STRING_LITERAL);
    ck_assert(string_token->line_number == 1);
    ck_assert(string_token->lexeme == source+strlen(" \"qwertyuiop\" "));
    ck_assert(string_token->lexeme_length == strlen("\"qwert\\\"yuiop\\\t\""));
}
END_TEST

START_TEST(hex_literal)
{
    const char * source = "0x123abc  0X123abc  0x123abcU 0X123abcu 0x123abcl 0x123abcL";
    Scanner * scanner = Scanner_init(source);

    int expected_lexeme_length[] = {8, 8, 9, 9, 9, 9};
    for(int i = 0;i < COUNT(expected_lexeme_length);i++)
    {
        Token * number_token = Scanner_get_next(scanner);
        printf("'%s'\n", number_token->lexeme);
        ck_assert(number_token->type == CONSTANT);
        ck_assert(number_token->line_number == 1);
        ck_assert(number_token->lexeme == source + i * 10);
        ck_assert(number_token->lexeme_length = expected_lexeme_length[i]);
    }
}
END_TEST

TCase * scanner_testcase(void)
{
    TCase * testcase = tcase_create("scanner");
    
    tcase_add_test(testcase, initialize_scanner);
    tcase_add_test(testcase, whitespace);
    tcase_add_test(testcase, single_character);
    tcase_add_test(testcase, assignments);
    tcase_add_test(testcase, string_literal);
    tcase_add_test(testcase, hex_literal);
    return testcase;
}





















