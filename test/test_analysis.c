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
static Symbol fake_symbol_a = {"*", &fake_type};
static Symbol fake_symbol_b = {"*", &fake_type};
static Symbol fake_symbol_c = {"*", &fake_type};

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
    expect_symbol_put(FAKE_SYMBOL_TABLE, "b2", &fake_symbol_a);
    expect_symbol_get(FAKE_SYMBOL_TABLE, "v", true, &fake_symbol_b);
    expect_symbol_get(FAKE_SYMBOL_TABLE, "a1", false, NULL);
    expect_symbol_put(FAKE_SYMBOL_TABLE, "a1", &fake_symbol_c);

    DeclAstNode *ast = parse_decl("int b2 = v;int a1;");
    analysis_ast_walk_decl(ast, FAKE_SYMBOL_TABLE);

    assert_true(ast->symbol == &fake_symbol_a);
    assert_true(ast->next->symbol == &fake_symbol_c);
}

static void symbol_lookup(void** state) {
    DeclAstNode *ast = parse_decl("void ignoreme(){return (a+(int)b) ? -c : (d = e[f](g,h));}");
    char *identifiers[] = {"a", "b", "c", "d", "e", "f", "g", "h"};
    for(int i = 0;i < 8;i++) {
        expect_symbol_get(FAKE_SYMBOL_TABLE, identifiers[i], true, &fake_symbol_a);
    }
    analysis_ast_walk_decl(ast, FAKE_SYMBOL_TABLE);
    ExprAstNode *ret = ast->body->block.head->return_jump.value;
    ExprAstNode *expr = ret->tertiary.condition_expr->binary.left;
    assert_true(expr->primary.symbol == &fake_symbol_a);
}

static void undeclared_symbol(void** state) {
    DeclAstNode *ast = parse_decl("void ignoreme(){return b;}");
    expect_symbol_get(FAKE_SYMBOL_TABLE, "b", true, NULL);
    expect_report_error(ANALYSIS, -1, "Undeclared identifier 'b'");

    analysis_ast_walk_decl(ast, FAKE_SYMBOL_TABLE);
}

static void previously_declared_symbol(void** state) {
    DeclAstNode *ast = parse_decl("void ignoreme(){int c;}");
    expect_symbol_get(FAKE_SYMBOL_TABLE, "c", false, &fake_symbol_a);
    expect_report_error(ANALYSIS, -1, "Previously declared identifier 'c'");

    analysis_ast_walk_decl(ast, FAKE_SYMBOL_TABLE);
}

int main(void) {
 const struct CMUnitTest tests[] = {
      cmocka_unit_test(declarations),
      cmocka_unit_test(symbol_lookup),
      cmocka_unit_test(undeclared_symbol),
      cmocka_unit_test(previously_declared_symbol)
  };

  return cmocka_run_group_tests(tests, NULL, NULL);
}