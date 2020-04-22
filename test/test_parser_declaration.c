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

static void primitive_declaration(void** state) {
  AstTestFixture tests[] = {

      // Integer types (signed by default)
      {"int c;", "(D [signed int], c)"},
      {"short int c;", "(D [signed short int], c)"},
      {"long int c;", "(D [signed long int], c)"},
      {"signed int c;", "(D [signed int], c)"},
      {"unsigned int c;", "(D [unsigned int], c)"},

      {"char q;", "(D [unsigned char], q)"},
      {"signed char t;", "(D [signed char], t)"},

      {"void diov;", "(D [void], diov)"},

      // Type qualifiers
      {"const char c;", "(D [const unsigned char], c)"},
      {"volatile int x;", "(D [volatile signed int], x)"},

      // Storage-class specifiers
      {"extern int abc;", "(D [extern signed int], abc)"},
      {"auto char a;", "(D [auto unsigned char], a)"},
      {"static int b;", "(D [static signed int], b)"},
      {"register void a;", "(D [register void], a)"},

      // Declarator lists.
      {"char a, b;", "(D [unsigned char], a, (D [unsigned char], b))"},

      {NULL, NULL}};
  assert_expected_ast_decl(tests);
}

static void pointer_declaration(void** state) {
  AstTestFixture tests[] = {

      {"char * c;", "(D [* [unsigned char]], c)"},
      {"short * p;", "(D [* [signed short int]], p)"},
      {"void ** q;", "(D [* [* [void]]], q)"},
      {"int a, *b;", "(D [signed int], a, (D [* [signed int]], b))"},

      {NULL, NULL}};
  assert_expected_ast_decl(tests);
}

static void array_declaration(void** state) {
  AstTestFixture tests[] = {

      {"char rahc[4];", "(D [[4] [unsigned char]], rahc)"},
      {"int * bc[2];", "(D [[2] [* [signed int]]], bc)"},
      {"int q[1][2];", "(D [[1] [[2] [signed int]]], q)"},
      {"void q, w[1];", "(D [void], q, (D [[1] [void]], w))"},

      {NULL, NULL}};
  assert_expected_ast_decl(tests);
}

static void grouped_type_declaration(void** state) {
  AstTestFixture tests[] = {

      {"char (* u);", "(D [* [unsigned char]], u)"},
      {"int (* u)[2];", "(D [* [[2] [signed int]]], u)"},
      {"int (* p[1][2])[3];", "(D [[1] [[2] [* [[3] [signed int]]]]], p)"},
      {"void *(*Q);", "(D [* [* [void]]], Q)"},

      {NULL, NULL}};
  assert_expected_ast_decl(tests);
}

static void function_type_declaration(void** state) {
  AstTestFixture tests[] = {
      {"char a();", "(D [f() [unsigned char]], a)"},
      {"int b(char a);", "(D [f([unsigned char]:a) [signed int]], b)"},
      {"void q(int b, char c);",
       "(D [f([signed int]:b,[unsigned char]:c) [void]], q)"},
      {"int (* q)();", "(D [* [f() [signed int]]], q)"},
      {"void *(*p)(int a);", "(D [* [f([signed int]:a) [* [void]]]], p)"},

      {NULL, NULL}};
  assert_expected_ast_decl(tests);
}

static void declaration_initializer(void** state) {
  AstTestFixture tests[] = {
      {"char a = 1;", "(D [unsigned char], a, (P 1))"},
      {"int a = 2*3;", "(D [signed int], a, (B (P 2), *, (P 3)))"},
      {"char* s = \"hw\";", "(D [* [unsigned char]], s, (P \"hw\"))"},
      {"int a=2, b=3;",
       "(D [signed int], a, (P 2), (D [signed int], b, (P 3)))"},

      {NULL, NULL}};
  assert_expected_ast_decl(tests);
}

static void abstract_declarators(void** state) {
  AstTestFixture tests[] = {
      {"void x(int);", "(D [f([signed int]:) [void]], x)"},
      {"int f(char, void*);",
       "(D [f([unsigned char]:,[* [void]]:) [signed int]], f)"},
      {"void x(int [23]);",
       "(D [f([[23] [signed int]]:) [void]], x)"},

      {NULL, NULL}};
  assert_expected_ast_decl(tests);
}

static void function_type_definitions(void** state) {
  AstTestFixture tests[] = {{"void p(){}", "(D [f() [void]], p, {B })"},

                            {NULL, NULL}};
  assert_expected_ast_decl(tests);
}

int main(void) {
  const struct CMUnitTest tests[] = {
      cmocka_unit_test(primitive_declaration),
      cmocka_unit_test(pointer_declaration),
      cmocka_unit_test(array_declaration),
      cmocka_unit_test(grouped_type_declaration),
      cmocka_unit_test(function_type_declaration),
      cmocka_unit_test(declaration_initializer),
      cmocka_unit_test(abstract_declarators),
      cmocka_unit_test(function_type_definitions)};

  return cmocka_run_group_tests(tests, NULL, NULL);
}
