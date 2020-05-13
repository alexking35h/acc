/*
 * Test the Context-sensitive Analysis compiler phase
 * (Arithmetic conversions)
 * 
 * These tests verify the implicit arithmetic conversions implemented
 * in the context-sensitive analysis phase.
 * 
 * Tests provide source code and populated symbol tables as input, and 
 * compare the AST afterwards to verify correct behaviour.
 * 
 * Conversions are detailed in section 6.3:
 * > Several operators convert operand values from one type to another type
 * > automatically. This subclause specifies the result required from such 
 * > an implicit conversion, as well as those that result from a cast operation.
 * 
 * Test:
 * - Integer promotion (6.3.1.1)
 * - Usual Arithmetic Conversions (6.3.1.8):
 *   - Common sign 
 *   - Different sign
 */
#include "ast.h"
#include "symbol.h"
#include "analysis.h"
#include "error.h"
#include "test.h"
#include "pretty_print.h"
#include "ctype.h"

#include <stdlib.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <cmocka.h>
#include <string.h>

#define FAKE_SYMBOL_TABLE ((SymbolTable*)0x1234)

#define CTYPE(x) ((CType){TYPE_BASIC, .basic.type_specifier = x})

static ExprAstNode* parse_expr(const char *source) {
    Scanner *scanner = Scanner_init(source);
    Parser *parser = Parser_init(scanner);
    return Parser_expression(parser);
}

static void symbol_table_add(char* name, CType* type) {
  Symbol* sym = calloc(1, sizeof(Symbol));
  sym->name = name;
  sym->type = type;
  expect_symbol_get(FAKE_SYMBOL_TABLE, name, true, sym);
}

static void integer_promotion(void** state) {
  ExprAstNode* ast = parse_expr("a+b");

  symbol_table_add("a", &CTYPE(TYPE_INT | TYPE_SIGNED));
  symbol_table_add("b", &CTYPE(TYPE_CHAR | TYPE_UNSIGNED));

  analysis_ast_walk_expr(ast, FAKE_SYMBOL_TABLE);

  char *expected = "(B (P a), +, (C [signed int], (P b)))";
  assert_true(test_compare_ast_expr(expected, ast));
}

static void arithmetic_conversions_common_sign(void** state) {
  ExprAstNode *ast = parse_expr("a-b*c");

  symbol_table_add("a", &CTYPE(TYPE_SIGNED_LONG_INT));
  symbol_table_add("b", &CTYPE(TYPE_SIGNED_INT));
  symbol_table_add("c", &CTYPE(TYPE_SIGNED_LONG_INT));
  
  analysis_ast_walk_expr(ast, FAKE_SYMBOL_TABLE);

  char *expected = "(B (P a), -, (B (C [signed long int], (P b)), *, (P c)))";
  assert_true(test_compare_ast_expr(expected, ast));
}

int main(void) {
 const struct CMUnitTest tests[] = {
      cmocka_unit_test(integer_promotion),
      cmocka_unit_test(arithmetic_conversions_common_sign),
  };

  return cmocka_run_group_tests(tests, NULL, NULL);
}