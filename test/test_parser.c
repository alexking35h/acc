#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include <cmocka.h>

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "error.h"
#include "parser.h"
#include "scanner.h"

void __wrap_Error_report_error(Error* error, ErrorType error_type,
                               int line_number, const char* error_string) {
  function_called();
  check_expected(error);
  check_expected(error_type);
  check_expected(line_number);
  check_expected(error_string);
}

static void assert_ast_matches(AstNode* ast_node, char* expected) {
  char ast_string[100];
  memset(ast_string, '\0', sizeof(ast_string));
  Ast_pretty_print(ast_node, ast_string, sizeof(ast_string));
  assert_string_equal(ast_string, expected);
}

static void initialize_parser(void** state) {
  Parser* parser = Parser_init(NULL, NULL);
  Parser_destroy(parser);
}

static void primary(void** state) {
  Scanner* scanner = Scanner_init("((heya))", NULL);
  Parser* parser = Parser_init(scanner, NULL);

  AstNode* ast_node = Parser_generate_ast(parser);
  assert_ast_matches(ast_node, "(PRIMARY $heya)");

  Parser_destroy(parser);
  Scanner_destroy(scanner);
}

static void postfix(void** state) {
  Scanner* scanner;
  Parser* parser;
  AstNode* ast_node;

  // Postfix ++ and -- operators
  scanner = Scanner_init("3++--", NULL);
  parser = Parser_init(scanner, NULL);

  ast_node = Parser_generate_ast(parser);
  assert_ast_matches(ast_node, "(POSTFIX (POSTFIX (PRIMARY 3), ++), --)");

  Parser_destroy(parser);
  Scanner_destroy(scanner);

  // Postfix . and -> operators.
  scanner = Scanner_init("A.->", NULL);
  parser = Parser_init(scanner, NULL);

  ast_node = Parser_generate_ast(parser);
  assert_ast_matches(ast_node, "(POSTFIX (POSTFIX (PRIMARY $A), .), ->)");

  Parser_destroy(parser);
  Scanner_destroy(scanner);

  // Postfix [] operator
  scanner = Scanner_init("a[\"2\"]", NULL);
  parser = Parser_init(scanner, NULL);

  ast_node = Parser_generate_ast(parser);
  assert_ast_matches(ast_node, "(POSTFIX (PRIMARY $a), (PRIMARY \"2\"))");

  Parser_destroy(parser);
  Scanner_destroy(scanner);
}

static void postfix_function_arguments(void** state) {
  // Postfix ()
  Scanner* scanner = Scanner_init("9()(3,4)", NULL);
  Parser* parser = Parser_init(scanner, NULL);

  AstNode* ast_node = Parser_generate_ast(parser);
  assert_ast_matches(
      ast_node,
      "(POSTFIX (POSTFIX (PRIMARY 9), (ARGUMENTS )), (ARGUMENTS 3, 4))");

  Parser_destroy(parser);
  Scanner_destroy(scanner);
}

static void unary(void ** state) {
  Scanner * scanner;
  Parser * parser;
  AstNode * ast_node;
  
  // ++/-- prefix unary operators.
  scanner = Scanner_init("++--1", NULL);
  parser = Parser_init(scanner, NULL);

  ast_node = Parser_generate_ast(parser);
  assert_ast_matches(ast_node, "(UNARY ++, (UNARY --, (PRIMARY 1)))");

  Parser_destroy(parser);
  Scanner_destroy(scanner);

  // sizeof prefix
  scanner = Scanner_init("sizeof a", NULL);
  parser = Parser_init(scanner, NULL);
  
  ast_node = Parser_generate_ast(parser);
  assert_ast_matches(ast_node, "(UNARY sizeof, (PRIMARY $a))");

  Parser_destroy(parser);
  Scanner_destroy(scanner);
}
  

int main(void) {
  const struct CMUnitTest tests[] = {
      cmocka_unit_test(initialize_parser), cmocka_unit_test(primary),
      cmocka_unit_test(postfix), cmocka_unit_test(postfix_function_arguments),
      cmocka_unit_test(unary)};
  return cmocka_run_group_tests(tests, NULL, NULL);
}
