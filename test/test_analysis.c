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
static Symbol fake_symbol_a = {"*", &fake_type};
static Symbol fake_symbol_b = {"*", &fake_type};
static Symbol fake_symbol_c = {"*", &fake_type};


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
    expect_get(FAKE_SYMBOL_TABLE, "b2", false, NULL);
    expect_put(FAKE_SYMBOL_TABLE, "b2", &fake_symbol_a);
    expect_get(FAKE_SYMBOL_TABLE, "v", true, &fake_symbol_b);
    expect_get(FAKE_SYMBOL_TABLE, "a1", false, NULL);
    expect_put(FAKE_SYMBOL_TABLE, "a1", &fake_symbol_c);

    DeclAstNode *ast = parse_decl("int b2 = v;int a1;");
    analysis_ast_walk_decl(ast, FAKE_SYMBOL_TABLE);

    assert_true(ast->symbol == &fake_symbol_a);
    assert_true(ast->next->symbol == &fake_symbol_c);
}

static void symbol_lookup(void** state) {
    DeclAstNode *ast = parse_decl("void ignoreme(){return (a+(int)b) ? -c : (d = e[f](g,h));}");
    char *identifiers[] = {"a", "b", "c", "d", "e", "f", "g", "h"};
    for(int i = 0;i < 8;i++) {
        expect_get(FAKE_SYMBOL_TABLE, identifiers[i], true, &fake_symbol_a);
    }
    analysis_ast_walk_decl(ast, FAKE_SYMBOL_TABLE);
    ExprAstNode *ret = ast->body->block.head->return_jump.value;
    ExprAstNode *expr = ret->tertiary.condition_expr->binary.left;
    assert_true(expr->primary.symbol == &fake_symbol_a);
}

static void undeclared_symbol(void** state) {
    DeclAstNode *ast = parse_decl("void ignoreme(){return b;}");
    expect_get(FAKE_SYMBOL_TABLE, "b", true, NULL);
    expect_report_error(ANALYSIS, -1, "Undeclared identifier 'b'");

    analysis_ast_walk_decl(ast, FAKE_SYMBOL_TABLE);
}

static void previously_declared_symbol(void** state) {
    DeclAstNode *ast = parse_decl("void ignoreme(){int c;}");
    expect_get(FAKE_SYMBOL_TABLE, "c", false, &fake_symbol_a);
    expect_report_error(ANALYSIS, -1, "Previously declared identifier 'c'");

    analysis_ast_walk_decl(ast, FAKE_SYMBOL_TABLE);
}

static void integer_promotion(void** state) {
    Scanner *scanner = Scanner_init("a+b");
    Parser *parser = Parser_init(scanner);
    ExprAstNode *ast = Parser_expression(parser);

    CType type_int = (CType){TYPE_PRIMITIVE, .primitive.type_specifier=TYPE_INT | TYPE_SIGNED};
    CType type_char = (CType){TYPE_PRIMITIVE, .primitive.type_specifier=TYPE_CHAR | TYPE_UNSIGNED};
    Symbol a = {"", &type_char}, b = {"", &type_int};
    expect_get(FAKE_SYMBOL_TABLE, "a", true, &a);
    expect_get(FAKE_SYMBOL_TABLE, "b", true, &b);

    analysis_ast_walk_expr(ast, FAKE_SYMBOL_TABLE);

    char ast_str[256] = "", expected_str[256] = "(B (C [signed int], (P a)), +, (P b))";
    pretty_print_expr(ast, ast_str, sizeof(ast_str));

    if(strlen(ast_str) != strlen(expected_str) || strcmp(ast_str, expected_str) != 0) {
      printf("Expected '%s', got '%s'\n", expected_str, ast_str);
      assert_true(false);
    }
}

int main(void) {
  const struct CMUnitTest tests[] = {
      cmocka_unit_test(declarations),
      cmocka_unit_test(symbol_lookup),
      cmocka_unit_test(undeclared_symbol),
      cmocka_unit_test(previously_declared_symbol),
      cmocka_unit_test(integer_promotion),
  };

  return cmocka_run_group_tests(tests, NULL, NULL);
}