/*
 * Test the context-sensitive analysis components.
 * 
 * These tests verify the context-sensitive analysis implemented
 * in analysis.c. This includes type-checking, and symbol table
 * generation.
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

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <cmocka.h>
#include <string.h>

#define FAKE_SYMBOL_TABLE ((SymbolTable*)0x1234)
#define FAKE_SYMBOL ((Symbol*)0x5678)

/* Mock symbol table functions */
SymbolTable* __wrap_symbol_table_create(SymbolTable* parent){
    check_expected(parent);
    return (SymbolTable*)mock();
}
Symbol* __wrap_symbol_table_put(SymbolTable* tab, char* name, CType* type) {
    if(!strcmp(name, "ignoreme")) return NULL;
    check_expected(tab);
    check_expected(name);
    return (Symbol*)mock();
}
Symbol* __wrap_symbol_table_get(SymbolTable* tab, char* name, bool search_parent) {
    if(!strcmp(name, "ignoreme")) return NULL;
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
static void expect_put(SymbolTable* tab, char* name, Symbol* ret) {
    expect_value(__wrap_symbol_table_put, tab, tab);
    expect_string(__wrap_symbol_table_put, name, name);
    will_return(__wrap_symbol_table_put, ret);
}
static void expect_get(SymbolTable* tab, char* name, bool search_parent, Symbol* ret) {
    expect_value(__wrap_symbol_table_get, tab, tab);
    expect_string(__wrap_symbol_table_get, name, name);
    expect_value(__wrap_symbol_table_get, search_parent, search_parent);
    will_return(__wrap_symbol_table_get, ret);
}

static DeclAstNode* parse(const char *source) {
    Scanner *scanner = Scanner_init(source);
    Parser *parser = Parser_init(scanner);
    DeclAstNode *node = Parser_translation_unit(parser);
    return node;
}

static void test_analysis_ast_walk(DeclAstNode* ast) {
    SymbolTable* global;

    expect_create(NULL, FAKE_SYMBOL_TABLE);
    analysis_ast_walk(ast, &global);
    assert_true(global == FAKE_SYMBOL_TABLE);
}

static void global_scope(void** state) {
    // A symbol table is created for 'global' scope,
    test_analysis_ast_walk(NULL);
}

static void declarations(void** state) {
    expect_get(FAKE_SYMBOL_TABLE, "b2", false, NULL);
    expect_put(FAKE_SYMBOL_TABLE, "b2", FAKE_SYMBOL);
    expect_get(FAKE_SYMBOL_TABLE, "v", true, FAKE_SYMBOL+1);
    expect_get(FAKE_SYMBOL_TABLE, "a1", false, NULL);
    expect_put(FAKE_SYMBOL_TABLE, "a1", FAKE_SYMBOL+2);

    DeclAstNode *ast = parse("int b2 = v;int a1;");
    test_analysis_ast_walk(ast);

    assert_true(ast->symbol == FAKE_SYMBOL);
    assert_true(ast->next->symbol == FAKE_SYMBOL+2);
}

static void symbol_lookup(void** state) {
    DeclAstNode *ast = parse("void ignoreme(){return (a+(int)b) ? -c : (d = e[f](g,h));}");
    char *identifiers[] = {"a", "b", "c", "d", "e", "f", "g", "h"};
    for(int i = 0;i < 8;i++) {
        expect_get(FAKE_SYMBOL_TABLE, identifiers[i], true, FAKE_SYMBOL+i);
    }
    test_analysis_ast_walk(ast);
    ExprAstNode *ret = ast->body->block.head->return_jump.value;
    ExprAstNode *expr = ret->tertiary.condition_expr->binary.left;
    assert_true(expr->primary.symbol == FAKE_SYMBOL);
}

static void undeclared_symbol(void** state) {
    DeclAstNode *ast = parse("void ignoreme(){return b;}");
    expect_get(FAKE_SYMBOL_TABLE, "b", true, NULL);
    expect_report_error(ANALYSIS, -1, "Undeclared identifier 'b'");

    test_analysis_ast_walk(ast);
}

int main(void) {
  const struct CMUnitTest tests[] = {
      cmocka_unit_test(global_scope),
      cmocka_unit_test(declarations),
      cmocka_unit_test(symbol_lookup),
      cmocka_unit_test(undeclared_symbol)
  };

  return cmocka_run_group_tests(tests, NULL, NULL);
}