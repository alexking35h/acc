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

/* Tests are organised into fixtures around ParserTestFixture,
 * which describes the expected AST string for a given source string. */
typedef struct ParserTestFixture {
  char* source;
  char* expected_ast;
} ParserTestFixture;

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

/* Assert the generated AST matches the expected AST for a given input */
static void assert_expected_ast(ParserTestFixture* fixture,
                                parser_function func) {
  for (; fixture->source; fixture++)
    assert_true(test_ast_equals_expected(fixture->source, fixture->expected_ast,
                                         Parser_expression));
}

static void initialize_parser(void** state) {
  Parser_destroy(Parser_init(NULL, NULL));
}

static void primary_expressions(void** state) {
  // Test simple primary expressions.
  ParserTestFixture tests[] = {{"1", "(PRIMARY 1)"},
                               {"q", "(PRIMARY q)"},
                               {"((3))", "(PRIMARY 3)"},
                               {"\"a\"", "(PRIMARY \"a\")"},
                               {NULL, NULL}};
  assert_expected_ast(tests, Parser_expression);
}

static void postfix_expressions(void** state) {
  // Test simple postfix expressions.
  ParserTestFixture tests[] = {
      {"a[1]", "(POSTFIX (PRIMARY a), (PRIMARY 1))"},
      {"a++", "(BINARY (PRIMARY a), =, (BINARY (PRIMARY a), +, (PRIMARY 1)))"},
      {"9--", "(BINARY (PRIMARY 9), =, (BINARY (PRIMARY 9), -, (PRIMARY 1)))"},
      {NULL, NULL}};

  assert_expected_ast(tests, Parser_expression);
}

static void unary_expressions(void** state) {
  // Test simple unary expressions
  ParserTestFixture tests[] = {
      {"++a", "(BINARY (PRIMARY a), =, (BINARY (PRIMARY a), +, (PRIMARY 1)))"},
      {"--b", "(BINARY (PRIMARY b), =, (BINARY (PRIMARY b), -, (PRIMARY 1)))"},
      {"&Q", "(UNARY &, (PRIMARY Q))"},
      {"*a", "(UNARY *, (PRIMARY a))"},
      {"+1", "(UNARY +, (PRIMARY 1))"},
      {"-a", "(UNARY -, (PRIMARY a))"},
      {"~9", "(UNARY ~, (PRIMARY 9))"},
      {"!1", "(UNARY !, (PRIMARY 1))"},
      {"sizeof 1", "(UNARY sizeof, (PRIMARY 1))"},
      {NULL, NULL}};
  assert_expected_ast(tests, Parser_expression);
}

static void multiplicative_expressions(void** state) {
  // Test multiplicative expressions (including right-associativity)
  ParserTestFixture tests[] = {
      {"1*2/3",
       "(BINARY (BINARY (PRIMARY 1), *, (PRIMARY 2)), /, (PRIMARY 3))"},
      {"1*2%3",
       "(BINARY (BINARY (PRIMARY 1), *, (PRIMARY 2)), %, (PRIMARY 3))"},
      {NULL, NULL}};
  assert_expected_ast(tests, Parser_expression);
}

static void additive_expressions(void** state) {
  // Test additive_expressions (including right-associativity)
  ParserTestFixture tests[] = {
      {"a+b-c",
       "(BINARY (BINARY (PRIMARY a), +, (PRIMARY b)), -, (PRIMARY c))"},
      {NULL, NULL}};
  assert_expected_ast(tests, Parser_expression);
}

static void shift_expressions(void** state) {
  // Test shift expressions (including right-associativity)
  ParserTestFixture tests[] = {
      {"a>>b<<c",
       "(BINARY (BINARY (PRIMARY a), >>, (PRIMARY b)), <<, (PRIMARY c))"},
      {NULL, NULL}};
  assert_expected_ast(tests, Parser_expression);
}

int main(void) {
  const struct CMUnitTest tests[] = {
      cmocka_unit_test(initialize_parser),
      cmocka_unit_test(primary_expressions),
      cmocka_unit_test(postfix_expressions),
      cmocka_unit_test(unary_expressions),
      cmocka_unit_test(multiplicative_expressions),
      cmocka_unit_test(additive_expressions),
      cmocka_unit_test(shift_expressions)};

  return cmocka_run_group_tests(tests, NULL, NULL);
}
