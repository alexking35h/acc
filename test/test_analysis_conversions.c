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

static void pointer_scaling_binary(void **state)
{
    // The 'pointer scale' field describes how the left or right
    // operand should be multiplied before the operation. E.g.:
    // _ptr_int * 3
    //
    // The right operand '3' should be multiplied by sizeof(int) - 4:
    // ptr_scale_right should equal 4. This should apply only to:
    // * +/- binary nodes
    // * binary nodes with one pointer/one integer only.

    // Not +/-, ptr_scale should not be set.
    ExprAstNode *ast = parse_expr("6 / 3");
    analysis_ast_walk_expr(NULL, ast, test_symbol_table);
    assert_true(ast->binary.ptr_scale_left == 0 && ast->binary.ptr_scale_right == 0);

    // Not a pointer. The 'ptr_scale' field should not be set.
    ast = parse_expr("_int + 1");
    analysis_ast_walk_expr(NULL, ast, test_symbol_table);
    assert_true(ast->binary.ptr_scale_left == 0 && ast->binary.ptr_scale_right == 0);

    ast = parse_expr("3 - _int");
    analysis_ast_walk_expr(NULL, ast, test_symbol_table);
    assert_true(ast->binary.ptr_scale_left == 0 && ast->binary.ptr_scale_right == 0);

    // Integer pointer. The 'ptr_scale' field should be set to four.
    ast = parse_expr("*(_ptr-100)");
    analysis_ast_walk_expr(NULL, ast, test_symbol_table);
    assert_true(ast->unary.right->binary.ptr_scale_right == 4);

    // Short pointer. The 'ptr_scale' field should be set to two.
    ast = parse_expr("*(_short_ptr+1000)");
    analysis_ast_walk_expr(NULL, ast, test_symbol_table);
    assert_true(ast->unary.right->binary.ptr_scale_right == 2);

    // char pointer. The 'ptr_scale' field should be set to one.
    ast = parse_expr("*(110+_char_ptr)");
    analysis_ast_walk_expr(NULL, ast, test_symbol_table);
    assert_true(ast->unary.right->binary.ptr_scale_left == 1);
}

static void pointer_scaling_unary(void **state)
{
    // Not a pointer. The 'ptr_scale' field should not be set.
    ExprAstNode *ast = parse_expr("++_int");
    analysis_ast_walk_expr(NULL, ast, test_symbol_table);
    assert_true(ast->unary.ptr_scale == 0 || ast->unary.ptr_scale == 1);

    // Integer pointer. The 'ptr_scale' field should be 4
    ast = parse_expr("++_ptr");
    analysis_ast_walk_expr(NULL, ast, test_symbol_table);
    assert_true(ast->unary.ptr_scale == 4);

    // Short pointer. The 'ptr_scale' field should be set to 2.
    ast = parse_expr("++_short_ptr");
    analysis_ast_walk_expr(NULL, ast, test_symbol_table);
    assert_true(ast->unary.ptr_scale == 2);

    // char pointer. The 'ptr_scale' field should be set to 1.
    ast = parse_expr("++_char_ptr");
    analysis_ast_walk_expr(NULL, ast, test_symbol_table);
    assert_true(ast->unary.ptr_scale == 1);
}

static void pointer_scaling_postfix(void **state)
{
    // Not a pointer. ptr_scale should not be set.
    ExprAstNode *ast = parse_expr("_int++");
    analysis_ast_walk_expr(NULL, ast, test_symbol_table);
    assert_true(ast->postfix.ptr_scale == 0 || ast->postfix.ptr_scale == 1);

    // Integer pointer. ptr_scale should be 4.
    ast = parse_expr("_ptr++");
    analysis_ast_walk_expr(NULL, ast, test_symbol_table);
    assert_true(ast->postfix.ptr_scale == 4);

    // Short pointer. ptr_scale should be 2.
    ast = parse_expr("_short_ptr++");
    analysis_ast_walk_expr(NULL, ast, test_symbol_table);
    assert_true(ast->postfix.ptr_scale == 2);

    // char pointer. ptr_scale should be 1.
    ast = parse_expr("_char_ptr++");
    analysis_ast_walk_expr(NULL, ast, test_symbol_table);
    assert_true(ast->postfix.ptr_scale == 1);
}

int main(void)
{
    test_symbol_table_setup();
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(integer_promotion),
        cmocka_unit_test(arithmetic_conversions_common_sign),
        cmocka_unit_test(pointer_scaling_binary),
        cmocka_unit_test(pointer_scaling_unary),
        cmocka_unit_test(pointer_scaling_postfix),
    };
    int ret = cmocka_run_group_tests(tests, NULL, NULL);

    test_symbol_table_teardown();
    return ret;
}