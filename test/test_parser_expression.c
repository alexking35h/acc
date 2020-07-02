#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <cmocka.h>

#include "ast.h"
#include "parser.h"
#include "scanner.h"
#include "test.h"

#define COUNT(x) ((sizeof(x)) / (sizeof(x[0])))

static void initialize_parser(void **state)
{
    Scanner *scanner = Scanner_init("", MOCK_ERROR_REPORTER);
    Parser_destroy(Parser_init(scanner, MOCK_ERROR_REPORTER));
    Scanner_destroy(scanner);
}

static void primary_expressions(void **state)
{
    // Test simple primary expressions.
    AstTestSet tests[] = {{"1", "(P 1)"},
                          {"q", "(P q)"},
                          {"((3))", "(P 3)"},
                          {"\"a\"", "(P \"a\")"},
                          {NULL, NULL}};
    assert_true(test_parse_compare_ast_set(tests, TEST_EXPR));
}

static void postfix_expressions(void **state)
{
    // Test postfix expressions.
    AstTestSet tests[] = {{"a[0]", "(U *, (B (P a), +, (P 0)))"},
                          {"a[3]", "(U *, (B (P a), +, (P 3)))"},
                          {"a++", "(PF (P a), ++)"},
                          {"9--", "(PF (P 9), --)"},
                          {"A()", "(F (P A))"},
                          {"q(1,2,3)", "(F (P q), (P 1), (P 2), (P 3))"},
                          {"(a+1)(b=1)", "(F (B (P a), +, (P 1)), (A (P b), (P 1)))"},
                          {NULL, NULL}};

    assert_true(test_parse_compare_ast_set(tests, TEST_EXPR));
}

static void unary_expressions(void **state)
{
    // Test unary expressions
    AstTestSet tests[] = {{"++a", "(U ++, (P a))"},
                          {"--b", "(U --, (P b))"},
                          {"&Q", "(U &, (P Q))"},
                          {"*a", "(U *, (P a))"},
                          {"+1", "(U +, (P 1))"},
                          {"-a", "(U -, (P a))"},
                          {"~9", "(U ~, (P 9))"},
                          {"!1", "(U !, (P 1))"},
                          {"sizeof 1", "(U sizeof, (P 1))"},
                          {NULL, NULL}};

    assert_true(test_parse_compare_ast_set(tests, TEST_EXPR));
}

static void binary_expressions(void **state)
{
    // Test binary mathematical operations (+, -, /, *, <<, >>)
    AstTestSet tests[] = {{"1*2/3", "(B (B (P 1), *, (P 2)), /, (P 3))"},
                          {"1*2%3", "(B (B (P 1), *, (P 2)), %, (P 3))"},
                          {"a>>b<<c", "(B (B (P a), >>, (P b)), <<, (P c))"},
                          {"a+b-c", "(B (B (P a), +, (P b)), -, (P c))"},
                          {NULL, NULL}};

    assert_true(test_parse_compare_ast_set(tests, TEST_EXPR));
}

static void comparison_expressions(void **state)
{
    AstTestSet tests[] = {{"2==T!=P", "(B (B (P 2), ==, (P T)), !=, (P P))"},
                          {"a<b>c", "(B (B (P a), <, (P b)), >, (P c))"},
                          {NULL, NULL}};

    assert_true(test_parse_compare_ast_set(tests, TEST_EXPR));
}

static void logical_expressions(void **state)
{
    // Test logical and bitwise expressions (&, ^, |, &&, ||) and tertiary
    // operator.
    AstTestSet tests[] = {{"1&2", "(B (P 1), &, (P 2))"},
                          {"a|b", "(B (P a), |, (P b))"},
                          {"a^b", "(B (P a), ^, (P b))"},
                          {"a&&b", "(B (P a), &&, (P b))"},
                          {"\"a\"||b", "(B (P \"a\"), ||, (P b))"},
                          {"a?1:3", "(T (P a), (P 1), (P 3))"},
                          {NULL, NULL}};

    assert_true(test_parse_compare_ast_set(tests, TEST_EXPR));
}

static void assignment_expressions(void **state)
{
    AstTestSet tests[] = {{"1*=2", "(A (P 1), (B (P 1), *, (P 2)))"},
                          {"1/=2", "(A (P 1), (B (P 1), /, (P 2)))"},
                          {"1%=2", "(A (P 1), (B (P 1), %, (P 2)))"},
                          {"1+=2", "(A (P 1), (B (P 1), +, (P 2)))"},
                          {"1-=2", "(A (P 1), (B (P 1), -, (P 2)))"},
                          {"1<<=2", "(A (P 1), (B (P 1), <<, (P 2)))"},
                          {"1>>=2", "(A (P 1), (B (P 1), >>, (P 2)))"},
                          {"1&=2", "(A (P 1), (B (P 1), &, (P 2)))"},
                          {"1^=2", "(A (P 1), (B (P 1), ^, (P 2)))"},
                          {"1|=2", "(A (P 1), (B (P 1), |, (P 2)))"},
                          {"1=2=3", "(A (P 1), (A (P 2), (P 3)))"},
                          {NULL, NULL}};

    assert_true(test_parse_compare_ast_set(tests, TEST_EXPR));
}

static void cast_expressions(void **state)
{
    AstTestSet tests[] = {{"(char)1", "(C [unsigned char], (P 1))"},

                          {NULL, NULL}};

    assert_true(test_parse_compare_ast_set(tests, TEST_EXPR));
}

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(initialize_parser),   cmocka_unit_test(primary_expressions),
        cmocka_unit_test(postfix_expressions), cmocka_unit_test(unary_expressions),
        cmocka_unit_test(binary_expressions),  cmocka_unit_test(comparison_expressions),
        cmocka_unit_test(logical_expressions), cmocka_unit_test(assignment_expressions),
        cmocka_unit_test(cast_expressions)};

    return cmocka_run_group_tests(tests, NULL, NULL);
}
