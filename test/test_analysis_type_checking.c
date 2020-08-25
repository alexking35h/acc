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
#include <stdlib.h>
#include <string.h>

#include <cmocka.h>

#include "analysis.h"
#include "ast.h"
#include "error.h"
#include "parser.h"
#include "scanner.h"
#include "test.h"

static ExprAstNode *parse_expr(const char *source)
{
    Scanner *scanner = Scanner_init(source, MOCK_ERROR_REPORTER);
    Parser *parser = Parser_init(scanner, MOCK_ERROR_REPORTER);
    return Parser_expression(parser);
}

static DeclAstNode *parse_decl(const char *source)
{
    Scanner *scanner = Scanner_init(source, MOCK_ERROR_REPORTER);
    Parser *parser = Parser_init(scanner, MOCK_ERROR_REPORTER);
    return Parser_declaration(parser);
}

static void primary(void **state)
{
    expect_report_error(ANALYSIS, 1, 0, "Undeclared identifier 'missing'");
    analysis_ast_walk_expr(MOCK_ERROR_REPORTER, parse_expr("missing"), test_symbol_table);

    ExprAstNode *ast = parse_expr("_function(_int) ? - _int : (_char = _ptr[_int])");
    analysis_ast_walk_expr(MOCK_ERROR_REPORTER, ast, test_symbol_table);
    assert_true(ast->tertiary.expr_true->unary.right->primary.symbol == _int);
}

static void postfix_operators(void **state)
{
    // Section 6.5.2:
    // - array subscripting
    // - function call
    // - ++ and -- operators

    // 6.5.2.1 (1): A postfix expression followed by an expression in square brackets
    // is a subscripted designation of an element of an array object. The definition
    // of the subscription operator is that E1[E2] is identical to (*((E1) + (E2))).
    // - Therefore, E1[E2] is equivalent to E2[E1].
    analysis_ast_walk_expr(MOCK_ERROR_REPORTER, parse_expr("_ptr[_int]"),
                           test_symbol_table);

    // 6.5.2.1 (2): Successive postscript operators designate an element of a
    // multidimensional array object.
    analysis_ast_walk_expr(MOCK_ERROR_REPORTER, parse_expr("_ptr_ptr[_int][_int]"),
                           test_symbol_table);

    // 6.5.2.2  (2): (In a function call) the number of arguments shall agree with the
    // number of parameters.
    expect_report_error(ANALYSIS, 1, 1,
                        "Invalid number of arguments to function. Expected 1, got 2");
    analysis_ast_walk_expr(MOCK_ERROR_REPORTER, parse_expr(" _function(_int, _int)"),
                           test_symbol_table);

    // 6.5.2.2 (2): (In a function call) Each argument shall have a type such that its
    // value may be assigned to an object with the unqualified version of the type of its
    // corresponding parameter.
    expect_report_error(ANALYSIS, 1, 10,
                        "Incompatible argument type. Cannot pass type 'pointer to int "
                        "signed' to type 'int signed'");
    analysis_ast_walk_expr(MOCK_ERROR_REPORTER, parse_expr("_function(_ptr)"),
                           test_symbol_table);

    // Error check - cannot call something that is not a function.
    expect_report_error(ANALYSIS, 1, 0, "Not a function");
    analysis_ast_walk_expr(MOCK_ERROR_REPORTER, parse_expr("_int(2,3)"),
                           test_symbol_table);

    // 6.5.2.4 (postfix increment operators) The operator shall have real or pointer type,
    // and shall be a modifiable l-value.
    expect_report_error(ANALYSIS, 1, 9, "Invalid operand type to postfix operator");
    analysis_ast_walk_expr(MOCK_ERROR_REPORTER, parse_expr("_function++"),
                           test_symbol_table);
    expect_report_error(ANALYSIS, 1, 0, "Invalid lvalue");
    analysis_ast_walk_expr(MOCK_ERROR_REPORTER, parse_expr("3++"), test_symbol_table);
    analysis_ast_walk_expr(MOCK_ERROR_REPORTER, parse_expr("_int++"), test_symbol_table);
    analysis_ast_walk_expr(MOCK_ERROR_REPORTER, parse_expr("_ptr++"), test_symbol_table);

    // 6.5.2.4 (postfix increment operators) The operator shall have real or pointer type,
    // and shall be a modifiable l-value.
    expect_report_error(ANALYSIS, 1, 9, "Invalid operand type to postfix operator");
    analysis_ast_walk_expr(MOCK_ERROR_REPORTER, parse_expr("_function--"),
                           test_symbol_table);
    expect_report_error(ANALYSIS, 1, 0, "Invalid lvalue");
    analysis_ast_walk_expr(MOCK_ERROR_REPORTER, parse_expr("3--"), test_symbol_table);
    analysis_ast_walk_expr(MOCK_ERROR_REPORTER, parse_expr("_int--"), test_symbol_table);
    analysis_ast_walk_expr(MOCK_ERROR_REPORTER, parse_expr("_ptr--"), test_symbol_table);
}

static void unary_operators(void **state)
{
    // Section 6.5.3 Unary operators
    // - '&' (address-of operator)
    // - '*' (dereference operator)
    // - '-' / '+' / '~' / '!'

    // 6.5.3.2 (1) The The operand of the unary '&' operand shall be either a function
    // designator, the result of a [] or unary * operator, or an lvalue that designates an
    // object (...)
    analysis_ast_walk_expr(MOCK_ERROR_REPORTER, parse_expr("*_ptr = 1"),
                           test_symbol_table);
    analysis_ast_walk_expr(MOCK_ERROR_REPORTER, parse_expr("**(&_ptr) = 1"),
                           test_symbol_table);
    expect_report_error(ANALYSIS, 1, 0, "Invalid Pointer dereference");
    analysis_ast_walk_expr(MOCK_ERROR_REPORTER, parse_expr("*2"), test_symbol_table);

    // 6.5.3.3 Unary arithmetic operators
    // 6.5.3.3 (1) The operand of the + or - operator shall have arithmetic type;
    // of the '~' operator, integer type; of the '!' operator, scalar type.
    expect_report_error(ANALYSIS, 1, 0, "Invalid operand to unary operator");
    analysis_ast_walk_expr(MOCK_ERROR_REPORTER, parse_expr("!_ptr"), test_symbol_table);
}

static void arithmetic_operators(void **state)
{
    // Multiplicative, additive, bitwise operators

    // 6.5.5 (multiplicative operators: *, / , %) Each of the operands shall have
    // arithmetic type.
    analysis_ast_walk_expr(MOCK_ERROR_REPORTER, parse_expr("_int * _char"),
                           test_symbol_table);
    expect_report_error(ANALYSIS, 1, 5, "Invalid operand type to binary operator");
    analysis_ast_walk_expr(MOCK_ERROR_REPORTER, parse_expr("_int * _ptr"),
                           test_symbol_table);
    expect_report_error(ANALYSIS, 1, 5, "Invalid operand type to binary operator");
    analysis_ast_walk_expr(MOCK_ERROR_REPORTER, parse_expr("_int / _ptr"),
                           test_symbol_table);
    expect_report_error(ANALYSIS, 1, 5, "Invalid operand type to binary operator");
    analysis_ast_walk_expr(MOCK_ERROR_REPORTER, parse_expr("_int % _ptr"),
                           test_symbol_table);

    // 6.5.6 (additive operators: +/i) For addition Either both operands shall have
    // arithmetic type, or one operand shall be a pointer to a complete object type and
    // the other shall have integer type
    analysis_ast_walk_expr(MOCK_ERROR_REPORTER, parse_expr("_int + _ptr_ptr"),
                           test_symbol_table);
    analysis_ast_walk_expr(MOCK_ERROR_REPORTER, parse_expr("_int + _ptr"),
                           test_symbol_table);
    analysis_ast_walk_expr(MOCK_ERROR_REPORTER, parse_expr("_int + _char"),
                           test_symbol_table);
    expect_report_error(ANALYSIS, 1, 5, "Invalid operand type to binary operator");
    analysis_ast_walk_expr(MOCK_ERROR_REPORTER, parse_expr("_ptr + _ptr"),
                           test_symbol_table);

    // 6.5.6 (additive operators: +/i) For subtraction, both operators can be pointers to
    // compatible object types, or the left operand is a pointer to a complete object type
    // and the right has integer type.
    analysis_ast_walk_expr(MOCK_ERROR_REPORTER, parse_expr("_int - _char"),
                           test_symbol_table);
    analysis_ast_walk_expr(MOCK_ERROR_REPORTER, parse_expr("_ptr - _ptr"),
                           test_symbol_table);
    analysis_ast_walk_expr(MOCK_ERROR_REPORTER, parse_expr("_ptr - _int"),
                           test_symbol_table);
    expect_report_error(ANALYSIS, 1, 5, "Invalid operand type to binary operator");
    analysis_ast_walk_expr(MOCK_ERROR_REPORTER, parse_expr("_ptr - _ptr_ptr"),
                           test_symbol_table);

    // 6.5.7 (bitwise shift operators) Each of the operands shall have integer type
    expect_report_error(ANALYSIS, 1, 6, "Invalid operand type to binary operator");
    analysis_ast_walk_expr(MOCK_ERROR_REPORTER, parse_expr("\"abc\" << 3"),
                           test_symbol_table);
}

static void comparison_operators(void **state)
{
    // <, > ==, <=, >=

    // 6.5.8 (Relationanal operators) Both operands have real type; or both are pointers
    // to compatible object types
    analysis_ast_walk_expr(MOCK_ERROR_REPORTER, parse_expr("_int < _char"),
                           test_symbol_table);
    analysis_ast_walk_expr(MOCK_ERROR_REPORTER, parse_expr("_int <= _char"),
                           test_symbol_table);
    analysis_ast_walk_expr(MOCK_ERROR_REPORTER, parse_expr("_int > _char"),
                           test_symbol_table);
    analysis_ast_walk_expr(MOCK_ERROR_REPORTER, parse_expr("_int >= _char"),
                           test_symbol_table);
    analysis_ast_walk_expr(MOCK_ERROR_REPORTER, parse_expr("_ptr < _ptr"),
                           test_symbol_table);
    analysis_ast_walk_expr(MOCK_ERROR_REPORTER, parse_expr("_ptr <= _ptr"),
                           test_symbol_table);
    analysis_ast_walk_expr(MOCK_ERROR_REPORTER, parse_expr("_ptr > _ptr"),
                           test_symbol_table);
    analysis_ast_walk_expr(MOCK_ERROR_REPORTER, parse_expr("_ptr >= _ptr"),
                           test_symbol_table);
    expect_report_error(ANALYSIS, 1, 5, "Invalid operand type to binary operator");
    analysis_ast_walk_expr(MOCK_ERROR_REPORTER, parse_expr("_ptr < _char"),
                           test_symbol_table);

    // 6.5.9 (equality operators) Both operators have arithmetic type, both operands are
    // pointers to compatible types
    analysis_ast_walk_expr(MOCK_ERROR_REPORTER, parse_expr("_int == _char"),
                           test_symbol_table);
    analysis_ast_walk_expr(MOCK_ERROR_REPORTER, parse_expr("_char != _int"),
                           test_symbol_table);
    analysis_ast_walk_expr(MOCK_ERROR_REPORTER, parse_expr("_ptr == _ptr"),
                           test_symbol_table);
    analysis_ast_walk_expr(MOCK_ERROR_REPORTER, parse_expr("_ptr != _ptr"),
                           test_symbol_table);
    expect_report_error(ANALYSIS, 1, 5, "Invalid operand type to binary operator");
    analysis_ast_walk_expr(MOCK_ERROR_REPORTER, parse_expr("_ptr != _int"),
                           test_symbol_table);
    expect_report_error(ANALYSIS, 1, 5, "Invalid operand type to binary operator");
    analysis_ast_walk_expr(MOCK_ERROR_REPORTER, parse_expr("_ptr != _ptr_ptr"),
                           test_symbol_table);

    expect_report_error(ANALYSIS, 1, 10, "Invalid operand type to binary operator");
    analysis_ast_walk_expr(MOCK_ERROR_REPORTER, parse_expr("_function == _ptr"),
                           test_symbol_table);
    analysis_ast_walk_expr(MOCK_ERROR_REPORTER, parse_expr("&_function && 98"),
                           test_symbol_table);
}

static void bitwise_operators(void **state)
{
    // &, |, ^  6.5.10, 6.5.11, 6.5.12 - operators must be arithmetic.
    analysis_ast_walk_expr(MOCK_ERROR_REPORTER,
                           parse_expr("_int & _char | _long_int ^ _int"),
                           test_symbol_table);
    expect_report_error(ANALYSIS, 1, 5, "Invalid operand type to binary operator");
    analysis_ast_walk_expr(MOCK_ERROR_REPORTER, parse_expr("_int & _ptr"),
                           test_symbol_table);
    expect_report_error(ANALYSIS, 1, 5, "Invalid operand type to binary operator");
    analysis_ast_walk_expr(MOCK_ERROR_REPORTER, parse_expr("_int | _ptr"),
                           test_symbol_table);
    expect_report_error(ANALYSIS, 1, 9, "Invalid operand type to binary operator");
    analysis_ast_walk_expr(MOCK_ERROR_REPORTER, parse_expr("_ptr_ptr ^ _long_int"),
                           test_symbol_table);
}

static void logical_operators(void **state)
{
    // &&, || 6.5.13, 6.5.14 - must be scalar (arithmetic & pointer)
    analysis_ast_walk_expr(MOCK_ERROR_REPORTER, parse_expr("_int && _char || _long_int"),
                           test_symbol_table);
    analysis_ast_walk_expr(MOCK_ERROR_REPORTER, parse_expr("_ptr && _char || _ptr"),
                           test_symbol_table);

    // ?: (6.5.15) The first operand must have scalar type (arithmetic/pointer).
    // The second operands must both be arithmetic; both be pointers to compatible types.
    analysis_ast_walk_expr(MOCK_ERROR_REPORTER, parse_expr("3?1:2"), test_symbol_table);
    analysis_ast_walk_expr(MOCK_ERROR_REPORTER, parse_expr("_ptr?_ptr:_ptr"),
                           test_symbol_table);

    expect_report_error(ANALYSIS, 1, 1, "Invalid types in tertiary expression");
    analysis_ast_walk_expr(MOCK_ERROR_REPORTER, parse_expr("1?_ptr:_int"),
                           test_symbol_table);
}

static void assignment_operators(void **state)
{
    // 6.5.16 (2) An assignment operator shall have a modifiable lvalue as its left
    // operand
    analysis_ast_walk_expr(MOCK_ERROR_REPORTER, parse_expr("_int = *_ptr = 1"),
                           test_symbol_table);

    expect_report_error(ANALYSIS, 1, 2, "Invalid lvalue");
    expect_report_error(ANALYSIS, 1, 14, "Invalid lvalue");
    expect_report_error(ANALYSIS, 1, 20, "Invalid lvalue");
    analysis_ast_walk_expr(MOCK_ERROR_REPORTER, parse_expr("1 ? 1 : 1 = 2 + 1 = 3 = 1"),
                           test_symbol_table);

    // 6.5.16.1 (2) The value of the right operand is converted to the type of the
    // assignment expression. Here, b (signed int), should be implicitly cast to char.
    ExprAstNode *ast = parse_expr("_char = _int");
    analysis_ast_walk_expr(MOCK_ERROR_REPORTER, ast, test_symbol_table);
    assert_true(ast->assign.right->type == CAST);
    assert_true(ast->assign.right->cast.to->basic.type_specifier == TYPE_UNSIGNED_CHAR);
    assert_true(ast->assign.right->cast.from->basic.type_specifier == TYPE_SIGNED_INT);

    // 6.5.16.1 Simple Assignment
    // - The left value has atomic, qualified, or unqualified arithmetic type, and the
    // right has arithmetic type.
    analysis_ast_walk_expr(MOCK_ERROR_REPORTER, parse_expr("_int = 1"),
                           test_symbol_table);

    // - The left value has pointer type, and both operators pointers compatible types.
    analysis_ast_walk_expr(MOCK_ERROR_REPORTER, parse_expr("_int = _int"),
                           test_symbol_table);

    // Incompatible assignment:
    expect_report_error(ANALYSIS, 1, 19,
                        "Incompatible assignment. Cannot assign type 'pointer to pointer "
                        "to int signed' to type 'pointer to int signed'");
    expect_report_error(ANALYSIS, 1, 12,
                        "Incompatible assignment. Cannot assign type 'pointer to int "
                        "signed' to type 'int signed'");
    expect_report_error(ANALYSIS, 1, 5,
                        "Incompatible assignment. Cannot assign type 'int signed' to "
                        "type 'pointer to int signed'");
    analysis_ast_walk_expr(MOCK_ERROR_REPORTER,
                           parse_expr("_ptr = _int = _ptr = _ptr_ptr"),
                           test_symbol_table);

    // Incompatible assignment:
    expect_report_error(ANALYSIS, 1, 5,
                        "Incompatible assignment. Cannot assign type 'pointer to "
                        "function returning int signed' "
                        "to type 'pointer to int signed'");
    analysis_ast_walk_expr(MOCK_ERROR_REPORTER, parse_expr("_ptr = &_function"),
                           test_symbol_table);
}

static void declaration_initializer(void **state)
{
    // 6.7.9 (Initialization) No initializer shall attempt to provide a value for an
    // object not contained within the entity being initialized.
    analysis_ast_walk_decl(NULL, parse_decl("int a = _int;"), test_symbol_table);
    expect_report_error(ANALYSIS, 1, 9,
                        "Invalid initializer value. Cannot assign type 'int signed' to "
                        "type 'pointer to int signed'");
    analysis_ast_walk_decl(MOCK_ERROR_REPORTER, parse_decl("int *b = _int;"),
                           test_symbol_table);
}

int main(void)
{
    test_symbol_table_setup();

    const struct CMUnitTest tests[] = {
        cmocka_unit_test(primary),
        cmocka_unit_test(postfix_operators),
        cmocka_unit_test(unary_operators),
        cmocka_unit_test(arithmetic_operators),
        cmocka_unit_test(comparison_operators),
        cmocka_unit_test(bitwise_operators),
        cmocka_unit_test(logical_operators),
        cmocka_unit_test(assignment_operators),
        cmocka_unit_test(declaration_initializer),
    };
    int result = cmocka_run_group_tests(tests, NULL, NULL);

    test_symbol_table_teardown();
    return result;
}