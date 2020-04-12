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



int main(void) {
  const struct CMUnitTest tests[] = {
      cmocka_unit_test(expression_statement)};

  return cmocka_run_group_tests(tests, NULL, NULL);
}
