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

static void expression_statement(void** state) {
  AstTestSet tests[] = {
      {"{x;}", "{B {E (P x)}}"},
      {"{a=1;}", "{B {E (A (P a), (P 1))}}"},
      {"{++a;}", "{B {E (A (P a), (B (P a), +, (P 1)))}}"},
      {"{(unsigned int)a+1;}",
       "{B {E (B (C [unsigned int], (P a)), +, (P 1))}}"},
      {"{a[0];}", "{B {E (U *, (B (P a), +, (P 0)))}}"},

      {NULL, NULL}};

  assert_true(test_parse_compare_ast_set(tests, TEST_STMT));
}

static void block_statement(void** state) {
  AstTestSet tests[] = {
      {"{}", "{B }"},
      {"{int y;}", "{B {D (D [signed int], y)}}"},
      {"{char p;p=1;}", "{B {D (D [unsigned char], p), {E (A (P p), (P 1))}}}"},
      {"{a=1; {a-1;}}",
       "{B {E (A (P a), (P 1)), {B {E (B (P a), -, (P 1))}}}}"},
      {"{{{a;}}}", "{B {B {B {E (P a)}}}}"},

      {NULL, NULL}};

  assert_true(test_parse_compare_ast_set(tests, TEST_STMT));
}

static void loops(void** state) {
  AstTestSet tests[] = {
      {"{while(x<1){}}", "{B {W (B (P x), <, (P 1)), {B }}}"},
      {"{while(x)g++;}", "{B {W (P x), {E (PF (P g), ++)}}}"},
      {"{while(1){int a=1;}}",
       "{B {W (P 1), {B {D (D [signed int], a, (P 1))}}}}"},

      {NULL, NULL}};

  assert_true(test_parse_compare_ast_set(tests, TEST_STMT));
}

static void return_statement(void** state) {
  AstTestSet tests[] = {
      {"{return;}", "{B {R }}"},
      {"{return 1;}", "{B {R (P 1)}}"},
      {"{return a*2;}", "{B {R (B (P a), *, (P 2))}}"},

      {NULL, NULL}};
 
  assert_true(test_parse_compare_ast_set(tests, TEST_STMT));
}

int main(void) {
  const struct CMUnitTest tests[] = {cmocka_unit_test(expression_statement),
                                     cmocka_unit_test(block_statement),
                                     cmocka_unit_test(loops),
                                     cmocka_unit_test(return_statement)};

  return cmocka_run_group_tests(tests, NULL, NULL);
}
