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

#define COUNT(x) ((sizeof(x)) / (sizeof(x[0])))

/* Tests are organised into fixtures around Fixture,
 * which describes the expected AST string for a given source string. */
typedef struct Fixture {
  char* source;
  char* expected_ast;
} Fixture;

/* Check if the AST for a given source string matches the expected AST */
static bool test_ast_equals_expected(char* source, char* expected) {
  // Generate Scanner and Parser objects.
  Scanner* scanner = Scanner_init(source, NULL);
  Parser* parser = Parser_init(scanner, NULL);

  // Generate the AST
  AstNode* ast_node = Parser_expression(parser);

  char generated_ast[256] = "";
  Ast_pretty_print(ast_node, generated_ast, sizeof(generated_ast));

  bool matches = (strlen(generated_ast) == strlen(expected) ||
                  strcmp(generated_ast, expected) == 0);

  if (!matches) {
    printf("%s FAIL. Expected '%s', got '%s'\n", __FUNCTION__, expected,
           generated_ast);
  }

  Parser_destroy(parser);
  Scanner_destroy(scanner);

  return matches;
}

/* Assert the generated AST matches the expected AST for a given input */
static void assert_expected_ast(Fixture* fixture) {
  for (; fixture->source; fixture++)
    assert_true(
        test_ast_equals_expected(fixture->source, fixture->expected_ast));
}

static void initialize_parser(void** state) {
  Parser_destroy(Parser_init(NULL, NULL));
}

static void primary_expressions(void** state) {
  // Test simple primary expressions.
  Fixture tests[] = {{"1", "(P 1)"},
                     {"q", "(P q)"},
                     {"((3))", "(P 3)"},
                     {"\"a\"", "(P \"a\")"},
                     {NULL, NULL}};
  assert_expected_ast(tests);
}

static void postfix_expressions(void** state) {
  // Test postfix expressions.
  Fixture tests[] = {{"a[1]", "(PF (P a), (P 1))"},
                     {"a++", "(A (P a), (B (P a), +, (P 1)))"},
                     {"9--", "(A (P 9), (B (P 9), -, (P 1)))"},
                     {NULL, NULL}};

  assert_expected_ast(tests);
}

static void unary_expressions(void** state) {
  // Test unary expressions
  Fixture tests[] = {{"++a", "(U ++, (P a))"},
                     {"--b", "(U --, (P b))"},
                     {"&Q", "(U &, (P Q))"},
                     {"*a", "(U *, (P a))"},
                     {"+1", "(U +, (P 1))"},
                     {"-a", "(U -, (P a))"},
                     {"~9", "(U ~, (P 9))"},
                     {"!1", "(U !, (P 1))"},
                     {"sizeof 1", "(U sizeof, (P 1))"},
                     {NULL, NULL}};
  assert_expected_ast(tests);
}

static void binary_expressions(void** state) {
  // Test binary mathematical operations (+, -, /, *, <<, >>)
  Fixture tests[] = {{"1*2/3", "(B (B (P 1), *, (P 2)), /, (P 3))"},
                     {"1*2%3", "(B (B (P 1), *, (P 2)), %, (P 3))"},
                     {"a>>b<<c", "(B (B (P a), >>, (P b)), <<, (P c))"},
                     {"a+b-c", "(B (B (P a), +, (P b)), -, (P c))"},
                     {NULL, NULL}};
  assert_expected_ast(tests);
}

static void comparison_expressions(void** state) {
  Fixture tests[] = {{"2==T!=P", "(B (B (P 2), ==, (P T)), !=, (P P))"},
                     {"a<b>c", "(B (B (P a), <, (P b)), >, (P c))"},
                     {NULL, NULL}};
  assert_expected_ast(tests);
}

static void logical_expressions(void** state) {
  // Test logical and bitwise expressions (&, ^, |, &&, ||) and tertiary
  // operator.
  Fixture tests[] = {{"1&2", "(B (P 1), &, (P 2))"},
                     {"a|b", "(B (P a), |, (P b))"},
                     {"a^b", "(B (P a), ^, (P b))"},
                     {"a&&b", "(B (P a), &&, (P b))"},
                     {"\"a\"||b", "(B (P \"a\"), ||, (P b))"},
                     {"a?1:3", "(T (P a), (P 1), (P 3))"},
                     {NULL, NULL}};
  assert_expected_ast(tests);
}

static void assignment_expressions(void** state) {
  Fixture tests[] = {{"1*=2", "(A (P 1), (B (P 1), *, (P 2)))"},
                     {"1/=2", "(A (P 1), (B (P 1), /, (P 2)))"},
                     {"1%=2", "(A (P 1), (B (P 1), %, (P 2)))"},
                     {"1+=2", "(A (P 1), (B (P 1), +, (P 2)))"},
                     {"1-=2", "(A (P 1), (B (P 1), -, (P 2)))"},
                     {"1<<=2", "(A (P 1), (B (P 1), <<, (P 2)))"},
                     {"1>>=2", "(A (P 1), (B (P 1), >>, (P 2)))"},
                     {"1&=2", "(A (P 1), (B (P 1), &, (P 2)))"},
                     {"1^=2", "(A (P 1), (B (P 1), ^, (P 2)))"},
                     {"1|=2", "(A (P 1), (B (P 1), |, (P 2)))"},
                     {"1=2=3", "(A (P a), (A (P 2), (P 3)))"},
                     {NULL, NULL}};

  assert_expected_ast(tests);
}

static void expressions(void** state) {
  Fixture tests[] = {
      {"1+1,2+4", "(E (B (P 1), +, (P 1)), (B (P 2), +, (P 4)))"},
      {NULL, NULL}};

  assert_expected_ast(tests);
}

int main(void) {
  const struct CMUnitTest tests[] = {cmocka_unit_test(initialize_parser),
                                     cmocka_unit_test(primary_expressions),
                                     cmocka_unit_test(postfix_expressions),
                                     cmocka_unit_test(unary_expressions),
                                     cmocka_unit_test(binary_expressions),
                                     cmocka_unit_test(comparison_expressions),
                                     cmocka_unit_test(logical_expressions),
                                     cmocka_unit_test(assignment_expressions),
                                     cmocka_unit_test(expressions)};

  return cmocka_run_group_tests(tests, NULL, NULL);
}
