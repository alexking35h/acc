/*
 * Context-sensitive analysis tests (type checking)
 * 
 * These unit tests verify the context-sensitive analysis implementation.
 * (analysis.h). This subset of tests verify type-checking functionality:
 *  - L-value type checking:
 *    - E.g. `3 = a` is invalid
 * 
 *  - operand type checking:
 *    - E.g. `a()` is valid only if `a` is a function.
 *    - E.g. `*a` is valid only if `a` is a pointer.
 *  
 *  - operand compatibility checking:
 *    - This applies to arithmetic operations/comparisons.
 *    - E.g. `a < 3` is invalid, if `a` is a pointer.
 * 
 * Correct behaviour is documented in the C11 standard in section 6.5 (expressions).
 * Each rule in the grammar has a subsection describing 'constraints' for the given
 * language rule.
 */
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <cmocka.h>
#include <string.h>
#include <stdlib.h>

#include "ast.h"
#include "error.h"
#include "scanner.h"
#include "parser.h"
#include "test.h"
#include "analysis.h"

static ExprAstNode* parse_expr(const char *source) {
    Scanner *scanner = Scanner_init(source);
    Parser *parser = Parser_init(scanner);
    return Parser_expression(parser);
}

static void primary(void** state) {
    expect_report_error(ANALYSIS, -1, "Undeclared identifier 'missing'");
    analysis_ast_walk_expr(parse_expr("missing"), test_symbol_table);

    ExprAstNode *ast = parse_expr("_function(_int) ? - _int : (_char = _ptr[_int])");
    analysis_ast_walk_expr(ast, test_symbol_table);
    assert_true(ast->tertiary.expr_true->unary.right->primary.symbol == _int);

}

static void postfix_operators(void** state) {
    // Section 6.5.2:
    // - array subscripting
    // - function call

    // 6.5.2.1 (1): A postfix expression followed by an expression in square brackets
    // is a subscripted designation of an element of an array object. The definition
    // of the subscription operator is that E1[E2] is identical to (*((E1) + (E2))).
    // - Therefore, E1[E2] is equivalent to E2[E1].
    analysis_ast_walk_expr(parse_expr("_ptr[_int]"), test_symbol_table);

    // 6.5.2.1 (2): Successive postscript operators designate an element of a 
    // multidimensional array object.
    analysis_ast_walk_expr(parse_expr("_ptr_ptr[_int][_int]"), test_symbol_table);

    // 6.5.2.2  (2): (In a function call) the number of arguments shall agree with the number
    // of parameters. 
    expect_report_error(ANALYSIS, -1, "Invalid number of arguments to function. Expected 1, got 2");
    analysis_ast_walk_expr(parse_expr("_function(_int, _int)"), test_symbol_table);

    // // 6.5.2.2 (2): (In a functionc all) Each argument shall have a type such that its value may be
    // // assigned to an object with the unqualified version of the type of its corresponding parameter.
    //  expect_report_error(
    //     ANALYSIS, 
    //     -1, 
    //     "Incompatible argument type. Cannot pass type 'pointer to signed int' to 'signed int'");
    //  analysis_ast_walk_expr(parse_expr("_function(_ptr)"), test_symbol_table);

    // Error check - cannot call something that is not a function.
    expect_report_error(ANALYSIS, -1, "Not a function");
    analysis_ast_walk_expr(parse_expr("_int(2,3)"), test_symbol_table);
}

static void unary_operators(void** state) {
    // Section 6.5.3 Unary operators
    // - '&' (address-of operator)
    // - '*' (dereference operator)
    // - '-' / '+' / '~' / '!'

    // 6.5.3.2 (1) The The operand of the unary '&' operand shall be either a function designator,
    // the result of a [] or unary * operator, or an lvalue that designates an object (...)
    analysis_ast_walk_expr(parse_expr("*_ptr = 1"), test_symbol_table);
    analysis_ast_walk_expr(parse_expr("**(&_ptr) = 1"), test_symbol_table);
    expect_report_error(ANALYSIS, -1, "Invalid Pointer dereference");
    analysis_ast_walk_expr(parse_expr("*2"), test_symbol_table);

    // 6.5.3.3 Unary arithmetic operators
    // 6.5.3.3 (1) The operand of the + or - operator shall have arithmetic type;
    // of the '~' operator, integer type; of the '!' operator, scalar type.
    expect_report_error(ANALYSIS, -1, "Invalid operand to unary operator '!'");
    analysis_ast_walk_expr(parse_expr("!_ptr"), test_symbol_table);
}

static void arithmetic_operators(void** state) {
    // Multiplicative, additive, bitwise operators
}

static void comparison_operators(void** state) {
    // ==, <=, >=
}

static void logical_operators(void** state) {
    // &&, || ?:
}

static void assignment_operators(void** state) {
    // 6.5.16 (2) An assignment operator shall have a modifiable lvalue as its left operand
    analysis_ast_walk_expr(parse_expr("_int = *_ptr = 1"), test_symbol_table);

    expect_report_error(ANALYSIS, -1, "Invalid lvalue");
    expect_report_error(ANALYSIS, -1, "Invalid lvalue");
    expect_report_error(ANALYSIS, -1, "Invalid lvalue");
    analysis_ast_walk_expr(parse_expr("1 ? 1 : 1 = 2 + 1 = 3 = 1"), test_symbol_table);

    // 6.5.16.1 (2) The value of the right operand is converted to the type of the assignment
    // expression. Here, b (signed int), should be implicitly cast to char.
    ExprAstNode* ast = parse_expr("_char = _int");
    analysis_ast_walk_expr(ast, test_symbol_table);
    assert_true(ast->assign.right->type == CAST);
    assert_true(ast->assign.right->cast.type->basic.type_specifier == TYPE_UNSIGNED_CHAR);

    // 6.5.16.1 Simple Assignment
    // - The left value has atomic, qualified, or unqualified arithmetic type, and the
    // right has arithmetic type.
    analysis_ast_walk_expr(parse_expr("_int = 1"), test_symbol_table);

    // - The left value has pointer type, and both operators pointers compatible types.
    analysis_ast_walk_expr(parse_expr("_int = _int"), test_symbol_table);

    // Incompatible assignment:
    // b = c - assigning pointer to primitive.
    // a = (b - c) - assigning primitive to a pointer.
    expect_report_error(
        ANALYSIS,
        -1,
        "Incompatible assignment. Cannot assign type 'pointer to pointer to int signed' to type 'pointer to int signed'"
    );
    expect_report_error(
        ANALYSIS,
        -1,
        "Incompatible assignment. Cannot assign type 'pointer to int signed' to type 'int signed'"
    );
    expect_report_error(
        ANALYSIS,
        -1,
        "Incompatible assignment. Cannot assign type 'int signed' to type 'pointer to int signed'"
    );
    analysis_ast_walk_expr(parse_expr("_ptr = _int = _ptr = _ptr_ptr"), test_symbol_table);
}

int main(void) {
    test_symbol_table_setup();

    const struct CMUnitTest tests[] = {
        cmocka_unit_test(primary),
        cmocka_unit_test(postfix_operators),
        cmocka_unit_test(unary_operators),
        cmocka_unit_test(arithmetic_operators),
        cmocka_unit_test(comparison_operators),
        cmocka_unit_test(logical_operators),
        cmocka_unit_test(assignment_operators),
    };
    int result = cmocka_run_group_tests(tests, NULL, NULL);

    test_symbol_table_teardown();
    return result;
}