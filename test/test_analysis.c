/*
 * Test the context-sensitive analysis components.
 * 
 * These tests verify the context-sensitive analysis implemented
 * in analysis.c/.h. This includes type-checking, and symbol table
 * generation.
 * 
 * Tests provide an AST as input, and mock the symbol table methods
 * create/get/put to verify correct behaviour.
 * 
 * Tests:
 * - global symbol table created + attached to empty translation unit
 * - function definitions added to global symbol table
 * - variable definitions added to global symbol table.
 */

#include "ast.h"
#include "symbol.h"
#include "analysis.h"

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <cmocka.h>

/* Mock symbol table functions */
SymbolTable* __wrap_symbol_table_create(SymbolTable* parent){
    check_expected(parent);
    return (SymbolTable*)mock();
}
void __wrap_symbol_table_put(SymbolTable* tab, char* name, CType* type) {
    check_expected(tab);
    check_expected(name);
    check_expected(type);
}
Symbol* __wrap_symbol_table_get(SymbolTable* tab, char* name, bool search_parent) {
    check_expected(tab);
    check_expected(name);
    check_expected(search_parent);
    return (Symbol*)mock();
}

/* Helper functions for checking mock function parameters and setting return values */
static void expect_create(SymbolTable* parent, SymbolTable* ret) {
    expect_value(__wrap_symbol_table_create, parent, parent);
    will_return(__wrap_symbol_table_create, ret);
}
static void expect_put(SymbolTable* tab, char* name, CType* type) {
    expect_value(__wrap_symbol_table_put, tab, tab);
    expect_value(__wrap_symbol_table_put, name, name);
    expect_value(__wrap_symbol_table_put, type, type);
}
static void expect_get(SymbolTable* tab, char* name, bool search_parent, Symbol* ret) {
    expect_value(__wrap_symbol_table_get, tab, tab);
    expect_value(__wrap_symbol_table_get, name, name);
    expect_value(__wrap_symbol_table_get, search_parent, search_parent);
    will_return(__wrap_symbol_table_get, ret);
}

static void global_scope_created(void** state) {
    // A symbol table is created for 'global' scope,
    expect_create(NULL, (SymbolTable*)0x1234);
    SymbolTable* global;
    analysis_ast_walk(NULL, &global);
    assert_true(global == (SymbolTable*)0x1234);
}

int main(void) {
  const struct CMUnitTest tests[] = {
      cmocka_unit_test(global_scope_created),
  };

  return cmocka_run_group_tests(tests, NULL, NULL);
}