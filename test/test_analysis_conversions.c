/*
 * Test the Context-sensitive Analysis compiler phase
 * (Arithmetic conversions)
 *
 * These tests verify the implicit arithmetic conversions implemented
 * in the context-sensitive analysis phase.
 *
 * Tests provide source code and populated symbol tables as input, and
 * compare the AST afterwards to verify correct behaviour.
 *
 * Conversions are detailed in section 6.3:
 * > Several operators convert operand values from one type to another type
 * > automatically. This subclause specifies the result required from such
 * > an implicit conversion, as well as those that result from a cast operation.
 *
 * Test:
 * - Integer promotion (6.3.1.1)
 * - Usual Arithmetic Conversions (6.3.1.8):
 *   - Common sign
 *   - Different sign
 */
#include "analysis.h"
#include "ast.h"
#include "ctype.h"
#include "error.h"
#include "parser.h"
#include "pretty_print.h"
#include "scanner.h"
#include "symbol.h"
#include "test.h"

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include <cmocka.h>

static ExprAstNode *parse_expr(const char *source)
{
    Scanner *scanner = Scanner_init(source, MOCK_ERROR_REPORTER);
    Parser *parser = Parser_init(scanner, MOCK_ERROR_REPORTER);
    return Parser_expression(parser);
}

static void integer_promotion(void **state)
{
    ExprAstNode *ast = parse_expr("_int+_char");
    analysis_ast_walk_expr(NULL, ast, test_symbol_table);

    char *expected = "(B (P _int), +, (C [signed int], (P _char)))";
    assert_true(test_compare_ast_expr(expected, ast));
}

static void arithmetic_conversions_common_sign(void **state)
{
    ExprAstNode *ast = parse_expr("_long_int - _int * _long_int");
    analysis_ast_walk_expr(NULL, ast, test_symbol_table);

    char *expected =
        "(B (P _long_int), -, (B (C [signed long int], (P _int)), *, (P _long_int)))";
    assert_true(test_compare_ast_expr(expected, ast));
}

int main(void)
{
    test_symbol_table_setup();
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(integer_promotion),
        cmocka_unit_test(arithmetic_conversions_common_sign),
    };
    int ret = cmocka_run_group_tests(tests, NULL, NULL);

    test_symbol_table_teardown();
    return ret;
}