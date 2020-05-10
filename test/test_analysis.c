/*
 * Test the Context-sensitive Analysis compiler phase
 * (Identifier declaration and lookup)
 * 
 * These tests verify the symbol table interactions implemented 
 * in analysis.c. This includes searching symbol tables for identifier
 * names, and declaring new identifiers.
 * 
 * Tests provide source code as input, and mock the symbol table methods
 * (create/put/get) to verify correct behaviour
 * 
 * Test:
 *  - symbol tables created for new scopes.
 *  - symbol look ups within statements, expressions, declarations
 *  - Error handling: undeclared symbol
 *  - Error handling: already declared symbol
 */

#include "ast.h"
#include "symbol.h"
#include "analysis.h"
#include "error.h"
#include "test.h"
#include "pretty_print.h"
#include "ctype.h"

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <cmocka.h>
#include <string.h>

#define FAKE_SYMBOL_TABLE ((SymbolTable*)0x1234)

static CType fake_type = {
    .type = TYPE_PRIMITIVE,
    .primitive.type_specifier = TYPE_INT | TYPE_SIGNED
};
static CType fake_ptr_type = {
    .type = TYPE_POINTER,
    .derived.type = &fake_type
};
static CType fake_ptr_ptr_type = {
    .type = TYPE_POINTER,
    .derived.type = &fake_ptr_type
};
static CType fake_function_type = {
    .type = TYPE_FUNCTION,
    .derived.type = &fake_type,
    .derived.params = &((ParameterListItem){NULL, &fake_type, NULL})
};
static Symbol fake_primitive = {"*", &fake_type};
static Symbol fake_ptr = {"*", &fake_ptr_type};
static Symbol fake_ptr_ptr = {"*", &fake_ptr_ptr_type};
static Symbol fake_function = {"*", &fake_function_type};

static DeclAstNode* parse_decl(const char *source) {
    Scanner *scanner = Scanner_init(source);
    Parser *parser = Parser_init(scanner);
    return Parser_translation_unit(parser);
}

static ExprAstNode* parse_expr(const char *source) {
    Scanner *scanner = Scanner_init(source);
    Parser *parser = Parser_init(scanner);
    return Parser_expression(parser);
}

static void declarations(void** state) {
    expect_symbol_get(FAKE_SYMBOL_TABLE, "b2", false, NULL);
    expect_symbol_put(FAKE_SYMBOL_TABLE, "b2", &fake_primitive);
    expect_symbol_get(FAKE_SYMBOL_TABLE, "v", true, &fake_primitive);
    expect_symbol_get(FAKE_SYMBOL_TABLE, "a1", false, NULL);
    expect_symbol_put(FAKE_SYMBOL_TABLE, "a1", &fake_primitive);

    DeclAstNode *ast = parse_decl("int b2 = v;int a1;");
    analysis_ast_walk_decl(ast, FAKE_SYMBOL_TABLE);

    assert_true(ast->symbol == &fake_primitive);
    assert_true(ast->next->symbol == &fake_primitive);
}

static void symbol_lookup(void** state) {
    DeclAstNode *ast = parse_decl("void ignoreme(){return (a(b)+(int)c) ? -d : (e = f[g]);}");
    expect_symbol_get(FAKE_SYMBOL_TABLE, "a", true, &fake_function);
    expect_symbol_get(FAKE_SYMBOL_TABLE, "b", true, &fake_primitive);
    expect_symbol_get(FAKE_SYMBOL_TABLE, "c", true, &fake_primitive);
    expect_symbol_get(FAKE_SYMBOL_TABLE, "d", true, &fake_primitive);
    expect_symbol_get(FAKE_SYMBOL_TABLE, "e", true, &fake_primitive);
    expect_symbol_get(FAKE_SYMBOL_TABLE, "f", true, &fake_ptr);
    expect_symbol_get(FAKE_SYMBOL_TABLE, "g", true, &fake_primitive);

    analysis_ast_walk_decl(ast, FAKE_SYMBOL_TABLE);
    ExprAstNode *ret = ast->body->block.head->return_jump.value;
    ExprAstNode *expr = ret->tertiary.expr_true->unary.right;
    assert_true(expr->primary.symbol == &fake_primitive);
}

static void undeclared_symbol(void** state) {
    DeclAstNode *ast = parse_decl("void ignoreme(){return b;}");
    expect_symbol_get(FAKE_SYMBOL_TABLE, "b", true, NULL);
    expect_report_error(ANALYSIS, -1, "Undeclared identifier 'b'");

    analysis_ast_walk_decl(ast, FAKE_SYMBOL_TABLE);
}

static void previously_declared_symbol(void** state) {
    DeclAstNode *ast = parse_decl("void ignoreme(){int c;}");
    expect_symbol_get(FAKE_SYMBOL_TABLE, "c", false, &fake_primitive);
    expect_report_error(ANALYSIS, -1, "Previously declared identifier 'c'");

    analysis_ast_walk_decl(ast, FAKE_SYMBOL_TABLE);
}

static void valid_lvalue(void** state) {
}

static void invalid_lvalue(void** state) {
}

static void assignment_operators(void** state) {
    // 6.5.16 (2) An assignment operator shall have a modifiable lvalue as its left operand
    expect_symbol_get(FAKE_SYMBOL_TABLE, "b", true, &fake_ptr);
    analysis_ast_walk_expr(parse_expr("a = *b = 1"), FAKE_SYMBOL_TABLE);

    ExprAstNode* ast = parse_expr("1 ? 1 : 1 = 2 + 1 = 3 = 1");
    expect_report_error(ANALYSIS, -1, "Invalid lvalue");
    expect_report_error(ANALYSIS, -1, "Invalid lvalue");
    expect_report_error(ANALYSIS, -1, "Invalid lvalue");
    analysis_ast_walk_expr(ast, FAKE_SYMBOL_TABLE);
}

static void postfix_operators(void** state) {
    // Section 6.5.2:
    // - array subscripting
    // - function call

    // 6.5.2.1 (1): A postfix expression followed by an expression in square brackets
    // is a subscripted designation of an element of an array object. The definition
    // of the subscription operator is that E1[E2] is identical to (*((E1) + (E2))).
    // - Therefore, E1[E2] is equivalent to E2[E1].
    expect_symbol_get(FAKE_SYMBOL_TABLE, "a", true, &fake_primitive);
    expect_symbol_get(FAKE_SYMBOL_TABLE, "b", true, &fake_ptr);
    analysis_ast_walk_expr(parse_expr("a[b]"), FAKE_SYMBOL_TABLE);

    // 6.5.2.1 (2): Successive postscript operators designate an element of a 
    // multidimensional array object.
    expect_symbol_get(FAKE_SYMBOL_TABLE, "c", true, &fake_ptr_ptr);
    analysis_ast_walk_expr(parse_expr("c[1000][2]"), FAKE_SYMBOL_TABLE);

    // 6.5.2.2  (2): (In a function call) the number of arguments shall agree with the number
    // of parameters. Each argument shall have a type such that its value may be assigned to an
    // object with the unqualified version of the type of its corresponding parameter.
    CType function_ctype = {
        TYPE_FUNCTION,
        .derived.type = &fake_type,
        .derived.params = &((ParameterListItem){NULL, &fake_type, NULL})
    };
    Symbol function_symbol = {"a", &function_ctype};
    expect_symbol_get(FAKE_SYMBOL_TABLE, "a", true, &function_symbol);
    expect_symbol_get(FAKE_SYMBOL_TABLE, "b", true, &fake_primitive);
    expect_symbol_get(FAKE_SYMBOL_TABLE, "c", true, &fake_primitive);
    expect_report_error(ANALYSIS, -1, "Invalid number of arguments to function. Expected 1, got 2");
    analysis_ast_walk_decl(parse_decl("void ignoreme(){ return a(b,c); }"), FAKE_SYMBOL_TABLE);
}

static void unary_operators(void** state) {
    // Section 6.5.3 Unary operators
    // - '&' (address-of operator)
    // - '*' (dereference operator)
    // - '-' / '+' / '~' / '!'

    // 6.5.3.2 (1) The The operand of the unary '&' operand shall be either a function designator,
    // the result of a [] or unary * operator, or an lvalue that designates an object (...)
    expect_symbol_get(FAKE_SYMBOL_TABLE, "a", true, &fake_ptr);
    analysis_ast_walk_expr(parse_expr("*a = 1"), FAKE_SYMBOL_TABLE);
    expect_symbol_get(FAKE_SYMBOL_TABLE, "a", true, &fake_ptr);
    analysis_ast_walk_expr(parse_expr("**(&a) = 1"), FAKE_SYMBOL_TABLE);
    expect_report_error(ANALYSIS, -1, "Invalid Pointer dereference");
    analysis_ast_walk_expr(parse_expr("*2"), FAKE_SYMBOL_TABLE);

    // 6.5.3.3 Unary arithmetic operators
    // 6.5.3.3 (1) The operand of the + or - operator shall have arithmetic type;
    // of the '~' operator, integer type; of the '!' operator, scalar type.
    expect_symbol_get(FAKE_SYMBOL_TABLE, "q", true, &fake_ptr);
    expect_report_error(ANALYSIS, -1, "Invalid operand to unary operator '!'");
    analysis_ast_walk_expr(parse_expr("!q"), FAKE_SYMBOL_TABLE);
}

int main(void) {
 const struct CMUnitTest tests[] = {
      cmocka_unit_test(declarations),
      cmocka_unit_test(symbol_lookup),
      cmocka_unit_test(undeclared_symbol),
      cmocka_unit_test(previously_declared_symbol),
      cmocka_unit_test(postfix_operators),
      cmocka_unit_test(unary_operators),
      cmocka_unit_test(assignment_operators),
  };

  return cmocka_run_group_tests(tests, NULL, NULL);
}