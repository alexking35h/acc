#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <cmocka.h>

#include "ast.h"
#include "parser.h"
#include "scanner.h"
#include "test.h"
#include "error.h"

static void panic_mode_declaration(void** state) {
  // Test panic mode behaviour at the top-level of a translation unit.
  // Should synchronise on the next semicolon:
  // void *q; int a a = 1; char b = 2 => void q*; char b = 2;
  const char* src = "void * q;int a a = 1; char b = 2;";
  const char* ast = "(D [* [void]], q, (D [unsigned char], b, (P 2)))";
  expect_report_error(1, "Expecting ';', got 'identifier'");
  assert_true(test_ast_compare_decl(src, ast));

  // Should synchronise with the next semicolon, or End of file.
  src = "void p; int a";
  ast = "(D [void], p)";
  expect_report_error(1, "Expecting ';', got 'End of File'");
  assert_true(test_ast_compare_decl(src, ast));

  // In this edgecase, there are no valid declarations that can be parsed.
  src = "void q int a = 2";
  expect_report_error(1, "Expecting ';', got 'int'");
  assert_true(test_ast_compare_decl(src, ""));

  // Test nested error-handling behaviour across code blocks.
  src = "char b; void p() {} int a";
  ast = "(D [unsigned char], b, (D [f() [void]], p, {B }))";
  expect_report_error(1, "Expecting ';', got 'End of File'");
  assert_true(test_ast_compare_decl(src, ast));

  // Parsing error in the initializer expression of a declaration.
  src = "int a = 3 + / 3;int b;";
  ast = "(D [signed int], b)";
  expect_report_error(1, "Expected expression, got '/'");
  assert_true(test_ast_compare_decl(src, ast));

  expect_report_error(1, "Expecting 'constant', got 'identifier'");
  assert_true(test_ast_compare_decl("int a[b];", ""));

  // Declarators must have an identifier (6.7.6)
  expect_report_error(1, "Missing identifier in declaration");
  assert_true(test_ast_compare_decl("int static long [23];", ""));
}

static void panic_mode_parameter_list(void** state) {
  // Test panic-mode behaviour in parameter declaration lists. In this
  // case, the parser should synchronise on ',' or ')'
  const char* src = "void p(int, plinth, char);";
  const char* ast = "(D [f([signed int]:,[unsigned char]:) [void]], p)";
  expect_report_error(1, "Invalid type");
  assert_true(test_ast_compare_decl(src, ast));

  src = "void w(int a,);";
  ast = "(D [f([signed int]:a) [void]], w)";
  expect_report_error(1, "Invalid type");
  assert_true(test_ast_compare_decl(src, ast));
}

static void panic_mode_compound_statement(void** state) {
  // Test panic-mode behaviour in compound statement. In this
  // case, the parser should synchronise on ';' or '}'.
  const char* src = "void p(){\na=1;b=2 a=3;\n}";
  const char* ast = "(D [f() [void]], p, {B {E (A (P a), (P 1))}})";
  expect_report_error(2, "Expecting ';', got 'identifier'");
  assert_true(test_ast_compare_decl(src, ast));

  src = "void p(){a= = 2;}";
  ast = "(D [f() [void]], p, {B })";
  expect_report_error(1, "Expected expression, got '='");
  assert_true(test_ast_compare_decl(src, ast));

  src = "void p(){\n  a=1\n}";
  ast = "(D [f() [void]], p, {B })";
  expect_report_error(3, "Expecting ';', got '}'");
  assert_true(test_ast_compare_decl(src, ast));
}

static void type_error(void** state) {
  // Test error-handling behaviour in type declarations.

  // Incalid type - missing type specifier
  // 6.7.2.1: At least one type specifier shall be given in the declaration
  // specifiers in each declaration.
  expect_report_error(1, "Invalid type");
  assert_true(test_ast_compare_decl("static volatile a;", ""));

  // Multiple storage-class specifiers.
  // 6.7.1.2 At most, one storage class specifier may be given in the declaration
  // specifiers in a declaration, except that _Thread_local may appear with static or extern
  expect_report_error(1, "Invalid type");
  assert_true(test_ast_compare_decl("static extern int a;", ""));

  // Invalid primitive type.
  expect_report_error(1, "Invalid type");
  assert_true(test_ast_compare_decl("short long int p;", ""));
  expect_report_error(1, "Invalid type");
  assert_true(test_ast_compare_decl("signed unsigned a;", ""));
  expect_report_error(1, "Invalid type");
  assert_true(test_ast_compare_decl("long char p;", ""));

  // Invalid type - functions cannot return functions.
  // 6.7.6.3: A function declarator shall not specify a return type that is a
  // function type or an array type.
  expect_report_error(1, "Functions cannot return functions (try Python?)");
  assert_true(test_ast_compare_decl("int a()();", ""));
  expect_report_error(1, "Functions cannot return arrays (try Python?)");
  assert_true(test_ast_compare_decl("int a()[1];", ""));

  // Type-names / abstract declarators are syntactically the same as normal
  // declarations with the identifier omitted (6.7.7)
  expect_report_error(1, "Type names must not have an identifier");
  assert_true(test_ast_compare_decl("int a = (char p[12])3;", ""));
}

int main(void) {
  const struct CMUnitTest tests[] = {
      cmocka_unit_test(panic_mode_declaration),
      cmocka_unit_test(panic_mode_parameter_list),
      cmocka_unit_test(panic_mode_compound_statement),
      cmocka_unit_test(type_error)};

  return cmocka_run_group_tests(tests, NULL, NULL);
}
