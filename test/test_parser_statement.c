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
      {"{x;}", "{B {E (P x)}}"},
      {"{a=1;}", "{B {E (A (P a), (P 1))}}"},
      {"{a++;}", "{B {E (A (P a), (B (P a), +, (P 1)))}}"},
      {"{(unsigned int)a+1;}",
       "{B {E (B (C [unsigned int], (P a)), +, (P 1))}}"},
      {"{a[0];}", "{B {E (PF (P a), (P 0))}}"},

      {NULL, NULL}};
  assert_expected_ast_stmt(tests);
}

static void block_statement(void** state) {
  AstTestFixture tests[] = {
      {"{}", "{B }"},
      {"{int y;}", "{B {D (D [signed int], y)}}"},
      {"{char p;p=1;}", "{B {D (D [unsigned char], p), {E (A (P p), (P 1))}}}"},
      {"{a=1; {a-1;}}",
       "{B {E (A (P a), (P 1)), {B {E (B (P a), -, (P 1))}}}}"},
      {"{{{a;}}}", "{B {B {B {E (P a)}}}}"},

      {NULL, NULL}};
  assert_expected_ast_stmt(tests);
}

static void loops(void** state) {
  AstTestFixture tests[] = {
      {"{while(x<1){}}", "{B {W (B (P x), <, (P 1)), {B }}}"},
      {"{while(x)g++;}", "{B {W (P x), {E (A (P g), (B (P g), +, (P 1)))}}}"},
      {"{while(1){int a=1;}}",
       "{B {W (P 1), {B {D (D [signed int], a, (P 1))}}}}"},

      {NULL, NULL}};
  assert_expected_ast_stmt(tests);
}

int main(void) {
  const struct CMUnitTest tests[] = {cmocka_unit_test(expression_statement),
                                     cmocka_unit_test(block_statement),
                                     cmocka_unit_test(loops)};

  return cmocka_run_group_tests(tests, NULL, NULL);
}
