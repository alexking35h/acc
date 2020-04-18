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

static void panic_mode_declaration(void** state) {
  // Test panic mode behaviour at the top-level of a translation unit.
  // Should synchronise on the next semicolon:
  // void *q; int a a = 1; char b = 2 => void q*; char b = 2;
  const char * src = "void * q;int a a = 1; char b = 2;";
  const char *ast = "(D [* [void]], q, (D [unsigned char], b, (P 2)))";
  assert_true(test_ast_compare_decl(src, ast));

  // Should synchronise with the next semicolon, or End of file.
  src = "void p; int a";
  ast = "(D [void], p)";
  assert_true(test_ast_compare_decl(src, ast));

  // In this edgecase, there are no valid declarations that can be parsed.
  src = "void q int a = 2";
  ast = "";
  assert_true(test_ast_compare_decl(src, ast));

  // Test same behaviour within code block.
  src = "void p(){int a a = 2;int a;}";
  ast = "(D [f() [void]], p, {B {D (D [signed int], a)}})";
  assert_true(test_ast_compare_decl(src, ast));
}

int main(void) {
  const struct CMUnitTest tests[] = {
      cmocka_unit_test(panic_mode_declaration)};

  return cmocka_run_group_tests(tests, NULL, NULL);
}
