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

static void primitive_declaration(void** state) {
  AstTestFixture tests[] = {

      // Integer types (signed by default)
      {"int c;", "(D [signed int], c)"},
      {"short int c;", "(D [signed short int], c)"},
      {"long int c;", "(D [signed long int], c)"},
      {"signed int c;", "(D [signed int], c)"},
      {"unsigned int c;", "(D [unsigned int], c)"},

      {"char q", "(D [unsigned char], q)"},
      {"signed char t;", "(D [signed char], t)"},

      {"void diov;", "(D [void], diov)"},

      // Type qualifiers
      {"const char c", "(D [const unsigned char], c)"},
      {"volatile int x", "(D [volatile signed int], x)"},

      // Storage-class specifiers
      {"extern int abc", "(D [extern signed int], abc)"},
      {"auto char a", "(D [auto unsigned char], a)"},
      {"static int b", "(D [static signed int], b)"},
      {"register void a", "(D [register void], a)"},

      {NULL, NULL}};
  assert_expected_ast_decl(tests);
}

static void pointer_declaration(void** state) {
  AstTestFixture tests[] = {

      // Simple pointers
      {"char * c;", "(D [* [unsigned char]], c)"},
      {"short * p;", "(D [* [signed short int]], p)"},
      {"void ** q;", "(D [* [* [void]]], q)"},

      {NULL, NULL}};
  assert_expected_ast_decl(tests);
}

static void array_declaration(void** state) {
  AstTestFixture tests[] = {
    
      // Array declarations
      {"char rahc[4]", "(D [[4] [unsigned char]], rahc)"},
      {"int * bc[2]", "(D [* [[a] [signed int]]], bc)"},
    
      {NULL, NULL}};
  assert_expected_ast_decl(tests);
}

int main(void) {
  const struct CMUnitTest tests[] = {cmocka_unit_test(primitive_declaration),
                                     cmocka_unit_test(pointer_declaration),
                                     cmocka_unit_test(array_declaration)};

  return cmocka_run_group_tests(tests, NULL, NULL);
}
