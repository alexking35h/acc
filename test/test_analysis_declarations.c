/*
 * Context-sensitive analysis tests (declarations)
 * 
 * These unit tests verify the context-sensitive analysis implementation
 * (analysis.h). This subset of tests verify address allocation functionality.
 * 
 * I.e., all non-function object declarations should be assigned an address
 * either statically (within the program's global address-space), or automatically
 * (on the stack).
 */
#include "test.h"

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>


#include <cmocka.h>

#include "scanner.h"
#include "parser.h"
#include "analysis.h"


#define MOCK_SYMBOL_TABLE (SymbolTable*)0x1234

/*
 * Mock symbol table functions.
 */
SymbolTable* __wrap_symbol_table_create(SymbolTable* parent) {
    function_called();
    check_expected(parent);
    return (SymbolTable*)mock();
}
Symbol* __wrap_symbol_table_put(SymbolTable* table, char* name, CType* type) {
    function_called();
    check_expected(table);
    check_expected(name);
    return (Symbol*)mock();
}
Symbol* __wrap_symbol_table_get(SymbolTable* table, char* name, bool search_parent) {
    function_called();
    check_expected(table);
    check_expected(name);
    check_expected(search_parent);
    return (Symbol*)mock();
}

static void expect_get(SymbolTable* table, char* name, _Bool search_parent, Symbol* return_value) {
    expect_function_call(__wrap_symbol_table_get);
    expect_value(__wrap_symbol_table_get, table, table);
    expect_string(__wrap_symbol_table_get, name, name);
    expect_value(__wrap_symbol_table_get, search_parent, search_parent);
    will_return(__wrap_symbol_table_get, return_value);
}
static void expect_put(SymbolTable* table, char* name, Symbol* return_value) {
    expect_function_call(__wrap_symbol_table_put);
    expect_value(__wrap_symbol_table_put, table, table);
    expect_string(__wrap_symbol_table_put, name, name);
    will_return(__wrap_symbol_table_put, return_value);
}

static DeclAstNode* parse_decl(char *src) {
    Scanner *scanner = Scanner_init(src);
    Parser *parser = Parser_init(scanner);
    return Parser_translation_unit(parser);
}

void basic_type(void** state) {
    // Test object declarations of basic types:
    // char, short, int, long.
    Symbol t1, t2, t3, t4, t5;
    expect_get(MOCK_SYMBOL_TABLE, "var1", false, NULL);
    expect_put(MOCK_SYMBOL_TABLE, "var1", &t1);
    expect_get(MOCK_SYMBOL_TABLE, "var2", false, NULL);
    expect_put(MOCK_SYMBOL_TABLE, "var2", &t2);
    expect_get(MOCK_SYMBOL_TABLE, "var3", false, NULL);
    expect_put(MOCK_SYMBOL_TABLE, "var3", &t3);
    expect_get(MOCK_SYMBOL_TABLE, "var4", false, NULL);
    expect_put(MOCK_SYMBOL_TABLE, "var4", &t4);
    expect_get(MOCK_SYMBOL_TABLE, "var5", false, NULL);
    expect_put(MOCK_SYMBOL_TABLE, "var5", &t5);
    DeclAstNode* decl = parse_decl("char var1;short var2;int var3;long int var4; char var5;");
    analysis_ast_walk_decl(decl, MOCK_SYMBOL_TABLE);

    assert_true(t1.address.type == ADDRESS_STATIC && t1.address.offset == 0);
    assert_true(t2.address.type == ADDRESS_STATIC && t2.address.offset == 2);
    assert_true(t3.address.type == ADDRESS_STATIC && t3.address.offset == 4);
    assert_true(t4.address.type == ADDRESS_STATIC && t4.address.offset == 8);
    assert_true(t5.address.type == ADDRESS_STATIC && t5.address.offset == 16);
}

void array_type(void** state) {
    // Test object declaration for array type.
    Symbol t1, t2;
    expect_get(MOCK_SYMBOL_TABLE, "arr1", false, NULL);
    expect_put(MOCK_SYMBOL_TABLE, "arr1", &t1);
    expect_get(MOCK_SYMBOL_TABLE, "var1", false, NULL);
    expect_put(MOCK_SYMBOL_TABLE, "var1", &t2);
    analysis_ast_walk_decl(parse_decl("int arr1[13];char var1;"), MOCK_SYMBOL_TABLE);

    assert_true(t1.address.type == ADDRESS_STATIC && t1.address.offset == 0);
    assert_true(t2.address.type == ADDRESS_STATIC && t2.address.offset == (4 * 13));
}

void ptr_type(void** state) {
    Symbol t1, t2;
    expect_get(MOCK_SYMBOL_TABLE, "ptr1", false, NULL);
    expect_put(MOCK_SYMBOL_TABLE, "ptr1", &t1);
    expect_get(MOCK_SYMBOL_TABLE, "var1", false, NULL);
    expect_put(MOCK_SYMBOL_TABLE, "var1", &t2);

    analysis_ast_walk_decl(parse_decl("int *ptr1;int var1;"), MOCK_SYMBOL_TABLE);

    assert_true(t1.address.type == ADDRESS_STATIC && t1.address.offset == 0);
    assert_true(t2.address.type == ADDRESS_STATIC && t2.address.offset == 4);
}

void automatic_allocation(void **state) {
    Symbol t1, f1, t2, t3;
    SymbolTable* fun_symbol_table = (SymbolTable*)0x5678;
    expect_get(MOCK_SYMBOL_TABLE, "var1", false, NULL);
    expect_put(MOCK_SYMBOL_TABLE, "var1", &t1);
    expect_get(MOCK_SYMBOL_TABLE, "fun1", false, NULL);
    expect_put(MOCK_SYMBOL_TABLE, "fun1", &f1);

    expect_function_call(__wrap_symbol_table_create);
    expect_value(__wrap_symbol_table_create, parent, MOCK_SYMBOL_TABLE);
    will_return(__wrap_symbol_table_create, fun_symbol_table);

    expect_get(fun_symbol_table, "arr1", false, NULL);
    expect_put(fun_symbol_table, "arr1", &t2);
    expect_get(fun_symbol_table, "var1", false, NULL);
    expect_put(fun_symbol_table, "var1", &t3);

    DeclAstNode* decl = parse_decl("int var1; void fun1(){int arr1[23];char var1;}");
    analysis_ast_walk_decl(decl, MOCK_SYMBOL_TABLE);

    assert_true(t2.address.type == ADDRESS_AUTOMATIC && t2.address.offset == 0);
    assert_true(t3.address.type == ADDRESS_AUTOMATIC && t3.address.offset == (23 * 4));
}

void previously_declared(void **state) {
    Symbol t1;
    expect_get(MOCK_SYMBOL_TABLE, "missing", false, &t1);
    expect_report_error(ANALYSIS, -1, "Previously declared identifier 'missing'");
    analysis_ast_walk_decl(parse_decl("int ** missing;"), MOCK_SYMBOL_TABLE);
}

void nested_function(void **state) {
    Symbol t1, t2;
    SymbolTable* fun_symbol_table = (SymbolTable*)0x5678;

    expect_get(MOCK_SYMBOL_TABLE, "outer", false, NULL);
    expect_put(MOCK_SYMBOL_TABLE, "outer", &t1);
    
    expect_function_call(__wrap_symbol_table_create);
    expect_value(__wrap_symbol_table_create, parent, MOCK_SYMBOL_TABLE);
    will_return(__wrap_symbol_table_create, fun_symbol_table);

    expect_report_error(ANALYSIS, -1, "Cannot have nested functions. Wait around for C30?");
    analysis_ast_walk_decl(parse_decl("void outer(){ int inner() {} }"), MOCK_SYMBOL_TABLE);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(basic_type),
        cmocka_unit_test(array_type),
        cmocka_unit_test(ptr_type),
        cmocka_unit_test(automatic_allocation),
        cmocka_unit_test(previously_declared),
        cmocka_unit_test(nested_function)
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}