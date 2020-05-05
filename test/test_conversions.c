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

#define CTYPE(x) ((CType){TYPE_PRIMITIVE, .primitive.type_specifier = x})

static void integer_promotion(void** state) {
    ExprAstNode* ast = parse_expr("a+b");

    symbol_table_add("a", &CTYPE(TYPE_INT | TYPE_SIGNED));
    symbol_table_add("b", &CTYPE(TYPE_CHAR | TYPE_UNSIGNED));

    analysis_ast_walk_expr(ast, FAKE_SYMBOL_TABLE);

    char ast_str[256] = "", expected_str[256] = "(B (P a), +, (C [signed int], (P b)))";
    pretty_print_expr(ast, ast_str, sizeof(ast_str));

    if(strlen(ast_str) != strlen(expected_str) || strcmp(ast_str, expected_str) != 0) {
      printf("Expected '%s', got '%s'\n", expected_str, ast_str);
      assert_true(false);
    }
}

static void arithmetic_conversions_common_sign(void** state) {
  ExprAstNode *ast = parse_expr("a-b*c");

  symbol_table_add("a", &CTYPE(TYPE_SIGNED_LONG_INT));
  symbol_table_add("b", &CTYPE(TYPE_SIGNED_INT));
  symbol_table_add("c", &CTYPE(TYPE_SIGNED_LONG_INT));
  
  analysis_ast_walk_expr(ast, FAKE_SYMBOL_TABLE);

  char ast_str[256] = "";
  char expected_str[256] = "(B (P a), -, (B (C [signed long int], (P b)), *, (P c)))";
  pretty_print_expr(ast, ast_str, sizeof(ast_str));

  if(strlen(ast_str) != strlen(expected_str) || strcmp(ast_str, expected_str) != 0) {
    printf("Expected '%s', got '%s'\n", expected_str, ast_str);
    assert_true(false);
  } 
}

// static void arithmetic_conversions_same_sign(void** state) {
//     // Test 'Usual arithmetic conversions' (6.3.1.8):
//     // * If both operands have signed integer types or both have unsigned integer
//     //   types, the operand with the type of lesser integer conversion rank is converted
//     //   to the operand of the type with greater rank
//     Scanner *scanner = Scanner_init("a-b*c");
//     Parser *parser = Parser_init(scanner);
//     ExprAstNode *ast = Parser_expression(parser);

//     CType type_long_int = {TYPE_PRIMITIVE, .primitive.type_specifier=TYPE_INT | TYPE_SIGNED | TYPE_LONG};
//     CType type_int = {TYPE_PRIMITIVE, .primitive.type_specifier=TYPE_INT | TYPE_SIGNED};
//     Symbol a = {"", &type_long_int}, b = {"", &type_int};
//     expect_get(FAKE_SYMBOL_TABLE, "a", true, &a);
//     expect_get(FAKE_SYMBOL_TABLE, "b", true, &b);

//     analysis_ast_walk_expr(ast, FAKE_SYMBOL_TABLE);

//     char ast_str[256], expected_str[256] = "(B (P a), -, (C [signed long int], (P b)))";
//     pretty_print_expr(ast, ast_str, sizeof(ast_str));

//     if(strlen(ast_str) != strlen(expected_str) || strcmp(ast_str, expected_str) != 0) {
//         printf("Expected '%s', got '%s'\n", expected_str, ast_str);
//         assert_true(false);
//     }
// }

// static void arithmetic_conversions_different_sign(void** state) {
//     // Test 'Usual arithmetic conversions' (6.3.1.8):
//     // * If the operand that has unsigned integer type has rank greater 
//     //   or equal to the rank of the type of the other operand, then the operand with
//     //   signed integer type is converted to the type of the operand with unsigned integer type.
//     Scanner *scanner = Scanner_init("a+b*c");
//     Parser *parser = Parser_init(scanner);
//     ExprAstNode *ast = Parser_expression(parser);

//     CType signed_long = {TYPE_PRIMITIVE, .primitive.type_specifier = TYPE_SIGNED_LONG_INT};
//     CType unsigned_long = {TYPE_PRIMITIVE, .primitive.type_specifier = TYPE_UNSIGNED_LONG_INT};
//     CType signed_int = {TYPE_PRIMITIVE, .primitive.type_specifier = TYPE_SIGNED_INT};

//     Symbol symbol_a = {"a", &signed_int};
//     Symbol symbol_b = {"b", &unsigned_long};
//     Symbol symbol_c = {"c", &signed_long};
//     expect_get(FAKE_SYMBOL_TABLE, "a", true, &symbol_a);
//     expect_get(FAKE_SYMBOL_TABLE, "b", true, &symbol_b);
//     expect_get(FAKE_SYMBOL_TABLE, "c", true, &symbol_c);

//     // c (signed_long) has equal rank to b (unsigned_long). So 'c' should
//     // be cast to (unsigned long). Then, a (int) is lower rank than
//     // long int, so a is cast to (unsigned long).
//     char ast_str[256], expected_str[256] = "(B (C (P a)), +, (B (P b), *, (C (P c))))";
//     pretty_print_expr(ast, ast_str, sizeof(ast_str));

//     if(strlen(ast_str) != strlen(ast_str) || strcmp(ast_str, expected_str) != 0) {
//         printf("Expected '%s', got '%s'\n'", expected_str, ast_str);
//         assert_true(false);
//     }

// }

int main(void) {
 const struct CMUnitTest tests[] = {
      cmocka_unit_test(integer_promotion),
      cmocka_unit_test(arithmetic_conversions_common_sign),
    //   cmocka_unit_test(arithmetic_-conversions_different_sign),
  };

  return cmocka_run_group_tests(tests, NULL, NULL);
}