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
 * Test:
 *  - symbol tables created for new scopes.
 *  - declarations for new symbols
 *  - symbol lookups within scope
 *  - Error handling: undeclared symbol
 *  - Error handling: already declared symbol
 * 
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
    expect_string(__wrap_symbol_table_put, name, name);
    expect_value(__wrap_symbol_table_put, type, type);
}
static void expect_get(SymbolTable* tab, char* name, bool search_parent, Symbol* ret) {
    expect_value(__wrap_symbol_table_get, tab, tab);
    expect_string(__wrap_symbol_table_get, name, name);
    expect_value(__wrap_symbol_table_get, search_parent, search_parent);
    will_return(__wrap_symbol_table_get, ret);
}

static void global_scope(void** state) {
    SymbolTable* global;

    // A symbol table is created for 'global' scope,
    expect_create(NULL, (SymbolTable*)0x1234);
    analysis_ast_walk(NULL, &global);
    assert_true(global == (SymbolTable*)0x1234);
}

static void declarations(void** state) {
    // A variable is added to global scope for each declaration
    // in the translation unit.
    DeclAstNode decl_2 = {
         .type = (CType*)0xabcd,
            .identifier = &((Token){.lexeme="testing456"})
    };
    DeclAstNode decl_1 = {
        .type = (CType*)0x3333,
        .identifier=&((Token){.lexeme="testing123"}),
        .next = &decl_2
    };
    expect_get((SymbolTable*)0x1234, "testing123", false, NULL);
    expect_put((SymbolTable*)0x1234, "testing123", (CType*)0x3333);
    expect_get((SymbolTable*)0x1234, "testing456", false, NULL);
    expect_put((SymbolTable*)0x1234, "testing456", (CType*)0xabcd);
    analysis_ast_walk_decl(&decl_1, (SymbolTable*)0x1234);
}

static void symbol_lookup_primary(void** state) {
    // A primary expression prompts a variable lookup within the current, and
    // parent symbol table.
    ExprAstNode primary = {
        .type = PRIMARY,
        .primary = {.identifier=&((Token){.lexeme="variable123"})}
    };
    expect_get((SymbolTable*)0x1234, "variable123", true, (Symbol*)0x9876);
    analysis_ast_walk_expr(&primary, (SymbolTable*)0x1234);

    // Unary expression prompts a variable lookup on the 'left'.
    ExprAstNode unary = {
        .type=UNARY,
        .unary.right = &primary
    };
    expect_get((SymbolTable*)0x1234, "variable123", true, (Symbol*)0x999);
    analysis_ast_walk_expr(&unary, (SymbolTable*)0x1234);
}

static void symbol_lookup_expr(void** state) {

}

int main(void) {
  const struct CMUnitTest tests[] = {
      cmocka_unit_test(global_scope),
      cmocka_unit_test(declarations),
      cmocka_unit_test(symbol_lookup_primary),
  };

  return cmocka_run_group_tests(tests, NULL, NULL);
}