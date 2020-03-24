#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include <cmocka.h>

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "ast.h"
#include "error.h"
#include "parser.h"
#include "scanner.h"

#define COUNT(x) ((sizeof(x)) / (sizeof(x[0])))

/* Typedef for parser functions, implemented in parser_*.c */
typedef AstNode* (*parser_function)(Parser* parser);

void __wrap_Error_report_error(Error* error, ErrorType error_type,
                               int line_number, const char* error_string) {
  function_called();
  check_expected(error);
  check_expected(error_type);
  check_expected(line_number);
  check_expected(error_string);
}

static bool test_ast_equals_expected(char* source, char* expected,
                                     parser_function function) {
  // Generate Scanner and Parser objects.
  Scanner* scanner = Scanner_init(source, NULL);
  Parser* parser = Parser_init(scanner, NULL);

  // Generate the AST
  AstNode* ast_node = function(parser);

  char generated_ast[256] = "";
  Ast_pretty_print(ast_node, generated_ast, sizeof(generated_ast));

  if (strlen(generated_ast) != strlen(expected) ||
      strcmp(generated_ast, expected)) {
    printf("%s FAIL. Expected '%s', got '%s'\n", __FUNCTION__, expected,
           generated_ast);
    return false;
  }
  return true;
}

static bool test_expression_ast_equals_expected(char* source, char* expected) {
  return test_ast_equals_expected(source, expected, Parser_expression);
}

static bool test_statement_ast_equals_expected(char* source, char* expected) {
  return test_ast_equals_expected(source, expected, Parser_statement);
}

static bool test_declaration_ast_equals_expected(char* source, char* expected) {
  return test_ast_equals_expected(source, expected, Parser_declaration);
}

static void initialize_parser(void** state) {
  Parser_destroy(Parser_init(NULL, NULL));
}

static void primary_expressions(void** state) {
  // Test simple primary expressions.
  struct {
    char* source;
    char* expected;
  } expr_tests[] = {{"1", "(PRIMARY 1)"},
                    {"q", "(PRIMARY q)"},
                    {"((3))", "(PRIMARY 3)"},
                    {"\"a\"", "(PRIMARY \"a\")"}};

  for (int i = 0; i < COUNT(expr_tests); i++) {
    char* source = expr_tests[i].source;
    char* expected = expr_tests[i].expected;

    assert_true(test_expression_ast_equals_expected(source, expected));
  }
}

static void postfix_expressions(void** state) {
  // Test simple postfix expressions.
  struct {
    char* source;
    char* expected;
  } expr_tests[] = {
      {"a[1]", "(POSTFIX (PRIMARY a), (PRIMARY 1))"},
      {"a++", "(BINARY (PRIMARY a), =, (BINARY (PRIMARY a), +, (PRIMARY 1)))"},
      {"9--", "(BINARY (PRIMARY 9), =, (BINARY (PRIMARY 9), -, (PRIMARY 1)))"}};

  for (int i = 0; i < COUNT(expr_tests); i++) {
    char* source = expr_tests[i].source;
    char* expected = expr_tests[i].expected;

    assert_true(test_expression_ast_equals_expected(source, expected));
  }
}

int main(void) {
  const struct CMUnitTest tests[] = {cmocka_unit_test(initialize_parser),
                                     cmocka_unit_test(primary_expressions),
                                     cmocka_unit_test(postfix_expressions)};
  return cmocka_run_group_tests(tests, NULL, NULL);
}
