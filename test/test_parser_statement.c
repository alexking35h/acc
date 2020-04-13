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

static void expression_statement(void** state) {
  AstTestFixture tests[] = {
      {"x;", "{E (P x)}"},
      {"a=1;", "{E (A (P a), (P 1))}"},
      {"a++;", "{E (A (P a), (B (P a), +, (P 1)))}"},
      {"(unsigned int)a+1;", "{E (B (C [unsigned int], (P a)), +, (P 1))}"},
      {"a[0];", "{E (PF (P a), (P 0))}"},

      {NULL, NULL}};
  assert_expected_ast_stmt(tests);
}

static void block_statement(void** state) {
  AstTestFixture tests[] = {
      {"{int y;}", "{B {D (D [signed int], y)}}"},
      {"{char p;p=1;}", "{B {D (D [unsigned char], p), {E (A (P p), (P 1))}}}"},
      {"{a=1; {a-1;}}",
       "{B {E (A (P a), (P 1)), {B {E (B (P a), -, (P 1))}}}}"},
      {"{{{a;}}}", "{B {B {B {E (P a)}}}}"},

      {NULL, NULL}};
  assert_expected_ast_stmt(tests);
}

int main(void) {
  const struct CMUnitTest tests[] = {cmocka_unit_test(expression_statement),
                                     cmocka_unit_test(block_statement)};

  return cmocka_run_group_tests(tests, NULL, NULL);
}