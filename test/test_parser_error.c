#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include <cmocka.h>

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "ast.h"
#include "parser.h"
#include "scanner.h"
#include "test.h"

void Error_report_error(Error* error, ErrorType error_type,
                               int line_number, const char* error_string) {
  function_called();
  check_expected(error);
  check_expected(error_type);
  check_expected(line_number);
  check_expected(error_string);
}

static void expect_report_error(
                             int expect_line,
                             char* expect_err_str) {
  expect_function_call(Error_report_error);
  expect_value(Error_report_error, error, 0x1234);
  expect_value(Error_report_error, error_type, PARSER);
  expect_value(Error_report_error, line_number, expect_line);
  expect_string(Error_report_error, error_string, expect_err_str);
}

static void panic_mode_declaration(void** state) {
  // Test panic mode behaviour at the top-level of a translation unit.
  // Should synchronise on the next semicolon:
  // void *q; int a a = 1; char b = 2 => void q*; char b = 2;
  const char * src = "void * q;int a a = 1; char b = 2;";
  const char *ast = "(D [* [void]], q, (D [unsigned char], b, (P 2)))";
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
  src = "void p() {} int a";
  ast = "(D [f() [void]], p, {B })";
  expect_report_error(1, "Expecting ';', got 'End of File'");
  assert_true(test_ast_compare_decl(src, ast));
}

int main(void) {
  const struct CMUnitTest tests[] = {
      cmocka_unit_test(panic_mode_declaration)};

  return cmocka_run_group_tests(tests, NULL, NULL);
}
