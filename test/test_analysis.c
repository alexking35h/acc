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
#include "error.h"

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <cmocka.h>
#include <string.h>

#define IDENT(p) &((Token){.type=IDENTIFIER, .lexeme=#p})
#define EXPR(...) ((ExprAstNode){__VA_ARGS__})
#define DECL(...) ((DeclAstNode){__VA_ARGS__})
#define EXPR_PRIMARY(x) \
    ((ExprAstNode){ \
        PRIMARY,\
        .primary.identifier=&((Token){.type=IDENTIFIER, .lexeme=#x}) \
    })

/* Mock symbol table functions */
SymbolTable* __wrap_symbol_table_create(SymbolTable* parent){
    check_expected(parent);
    return (SymbolTable*)mock();
}
Symbol* __wrap_symbol_table_put(SymbolTable* tab, char* name, CType* type) {
    if(!strcmp(name, "ignoreme")) return NULL;
    check_expected(tab);
    check_expected(name);
    check_expected(type);
    return (Symbol*)mock();
}
Symbol* __wrap_symbol_table_get(SymbolTable* tab, char* name, bool search_parent) {
    if(!strcmp(name, "ignoreme")) return NULL;
    check_expected(tab);
    check_expected(name);
    check_expected(search_parent);
    return (Symbol*)mock();
}

/* Mock error functions */
void __wrap_Error_report_error(ErrorType type, int line, const char * msg) {
    check_expected(type);
    check_expected(line);
    check_expected(msg);
}

/* Helper functions for checking mock function parameters and setting return values */
static void expect_create(SymbolTable* parent, SymbolTable* ret) {
    expect_value(__wrap_symbol_table_create, parent, parent);
    will_return(__wrap_symbol_table_create, ret);
}
static void expect_put(SymbolTable* tab, char* name, CType* type, Symbol* ret) {
    if(strcmp(name, "ignoreme") == 0) return;
    expect_value(__wrap_symbol_table_put, tab, tab);
    expect_string(__wrap_symbol_table_put, name, name);
    expect_value(__wrap_symbol_table_put, type, type);
    will_return(__wrap_symbol_table_put, ret);
}
static void expect_get(SymbolTable* tab, char* name, bool search_parent, Symbol* ret) {
    expect_value(__wrap_symbol_table_get, tab, tab);
    expect_string(__wrap_symbol_table_get, name, name);
    expect_value(__wrap_symbol_table_get, search_parent, search_parent);
    will_return(__wrap_symbol_table_get, ret);
}

static void test_walk_decl(DeclAstNode* node) {
    SymbolTable* tmp;
    expect_create(NULL, (SymbolTable*)0x1234);
    analysis_ast_walk(node, &tmp);
}

static void test_walk_expr(ExprAstNode* node) {
    DeclAstNode decl = {
        .type=&((CType){TYPE_PRIMITIVE}),
        .identifier=IDENT(ignoreme),
        .initializer=node
    };
    test_walk_decl(&decl);
}

static void test_walk_stmt(StmtAstNode* node) {
    DeclAstNode decl = {
        .type=&((CType){TYPE_FUNCTION}),
        .identifier=IDENT(ignoreme),
        .body = node
    };
    test_walk_decl(&decl);
}


static void global_scope(void** state) {
    // A symbol table is created for 'global' scope,
    SymbolTable* global;

    expect_create(NULL, (SymbolTable*)0x1234);
    analysis_ast_walk(NULL, &global);
    assert_true(global == (SymbolTable*)0x1234);
}

static void declarations(void** state) {
    // A variable is added to global scope for each declaration
    // in the translation unit. This AST corresponds to: int b2 = v; int a1;
    CType prim1 = {TYPE_PRIMITIVE}, prim2 = {TYPE_PRIMITIVE};
    DeclAstNode decl_2 = {.type = &prim1, .identifier = IDENT(a1)};
    DeclAstNode decl_1 = {
        .type = &prim2,
        .identifier=IDENT(b2),
        .next = &decl_2,
        .initializer=&EXPR_PRIMARY(v)
    };
    expect_get((SymbolTable*)0x1234, "b2", false, NULL);
    expect_put((SymbolTable*)0x1234, "b2", &prim2, (Symbol*)0x1234);
    expect_get((SymbolTable*)0x1234, "v", true, (Symbol*)1);
    expect_get((SymbolTable*)0x1234, "a1", false, NULL);
    expect_put((SymbolTable*)0x1234, "a1", &prim1, (Symbol*)0x2345);
    test_walk_decl(&decl_1);

    // Make sure that the declaration node has been anotated with the symbol returned
    // from symbol_table_get
    assert_true(decl_2.symbol == (Symbol*)0x2345);
    assert_true(decl_1.symbol == (Symbol*)0x1234);
}

static void symbol_lookup_expr(void** state) {
    // Each of the node in the expression tree should be visited in a post-order walk.
    // This AST represents something like '(a+(int)b) ? -c : d = e'
    ExprAstNode primary = EXPR_PRIMARY(a);
    ExprAstNode cast = EXPR(CAST, .cast.right=&EXPR_PRIMARY(b));
    ExprAstNode binary = EXPR(BINARY, .binary={.left=&primary, .right=&cast});
    ExprAstNode unary = EXPR(UNARY, .unary.right=&EXPR_PRIMARY(c));
    ExprAstNode assign = EXPR(ASSIGN, .assign={&EXPR_PRIMARY(d), &EXPR_PRIMARY(e)});
    ExprAstNode tertiary = {
        .type=TERTIARY,
        .tertiary = {&binary, &unary, &assign}
    };

    // Make sure that we called symbol_table_get for each symbol in post-order walk.
    for(int i = 0;i < 5;i++) {
        expect_get((SymbolTable*)0x1234, (char*[]){"a","b","c","d","e"}[i], true, (Symbol*)21);
    }
    test_walk_expr(&tertiary);

    // Check that at least the first primary node has been annotated with the 
    // result of symbol_table_get
    assert_true(primary.primary.symbol == (Symbol*)21);
}

static void symbol_lookup_postfix(void** state) {
    // Every node in the postfix expression should be visited in the post-order
    // walk. This expression looks like: a[b](i,j)
    ExprAstNode array = EXPR(
        POSTFIX,
        .postfix={
            .left=&EXPR_PRIMARY(a),
            .index_expression=&EXPR_PRIMARY(b)
        }
    );
    ArgumentListItem args = {
        &EXPR_PRIMARY(i),
        &((ArgumentListItem){&EXPR_PRIMARY(j)})
    };
    ExprAstNode fun = EXPR(
        POSTFIX,
        .postfix={
            .left=&array,
            .args=&args
        }
    );
    expect_get((SymbolTable*)0x1234, "a", true, (Symbol*)1);
    expect_get((SymbolTable*)0x1234, "b", true, (Symbol*)1);
    expect_get((SymbolTable*)0x1234, "i", true, (Symbol*)1);
    expect_get((SymbolTable*)0x1234, "j", true, (Symbol*)1);
    test_walk_expr(&fun);
}

static void symbol_lookup_stmt(void** state) {
    CType prim = {TYPE_PRIMITIVE};
    ExprAstNode expr = EXPR_PRIMARY(a);
    StmtAstNode expr_stmt = {EXPR, .expr.expr=&expr};
    StmtAstNode decl_stmt = {
        DECL,
        .decl.decl=&DECL(.type=&prim,.identifier=IDENT(b)),
        .next=&expr_stmt
    };
    StmtAstNode ret = {
        RETURN_JUMP,
        .return_jump.value=&EXPR_PRIMARY(c),
        .next = &decl_stmt
    };
    expect_get((SymbolTable*)0x1234, "c", true, (Symbol*)1);
    expect_get((SymbolTable*)0x1234, "b", false, NULL);
    expect_put((SymbolTable*)0x1234, "b", &prim, (Symbol*)1);
    expect_get((SymbolTable*)0x1234, "a", true, (Symbol*)1);
    test_walk_stmt(&ret);
}

static void undeclared_symbol(void** state) {
    ExprAstNode prim = EXPR_PRIMARY(m);
    expect_get((SymbolTable*)0x1234, "m", true, NULL);
    expect_value(__wrap_Error_report_error, type, ANALYSIS);
    expect_value(__wrap_Error_report_error, line, -1);
    expect_string(__wrap_Error_report_error, msg, "Undeclared identifier 'm'");
    test_walk_expr(&prim);
}

int main(void) {
  const struct CMUnitTest tests[] = {
      cmocka_unit_test(global_scope),
      cmocka_unit_test(declarations),
      cmocka_unit_test(symbol_lookup_expr),
      cmocka_unit_test(symbol_lookup_postfix),
      cmocka_unit_test(symbol_lookup_stmt),
      cmocka_unit_test(undeclared_symbol)
  };

  return cmocka_run_group_tests(tests, NULL, NULL);
}