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

/* Check if the AST for a given source string matches the expected AST */
static bool test_ast_equals_expected(char* source, char* expected,
                                     parser_function function) {
  // Generate Scanner and Parser objects.
  Scanner* scanner = Scanner_init(source, NULL);
  Parser* parser = Parser_init(scanner, NULL);

  // Generate the AST
  AstNode* ast_node = function(parser);

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
  ParserTestFixture tests[] = {{"1", "(P 1)"},
                               {"q", "(P q)"},
                               {"((3))", "(P 3)"},
                               {"\"a\"", "(P \"a\")"},
                               {NULL, NULL}};
  assert_expected_ast(tests, Parser_expression);
}

static void postfix_expressions(void** state) {
  // Test simple postfix expressions.
  ParserTestFixture tests[] = {{"a[1]", "(PF (P a), (P 1))"},
                               {"a++", "(A (P a), (B (P a), +, (P 1)))"},
                               {"9--", "(A (P 9), (B (P 9), -, (P 1)))"},
                               {NULL, NULL}};

  assert_expected_ast(tests, Parser_expression);
}

static void unary_expressions(void** state) {
  // Test simple unary expressions
  ParserTestFixture tests[] = {{"++a", "(U ++, (P a))"},
                               {"--b", "(U --, (P b))"},
                               {"&Q", "(U &, (P Q))"},
                               {"*a", "(U *, (P a))"},
                               {"+1", "(U +, (P 1))"},
                               {"-a", "(U -, (P a))"},
                               {"~9", "(U ~, (P 9))"},
                               {"!1", "(U !, (P 1))"},
                               {"sizeof 1", "(U sizeof, (P 1))"},
                               {NULL, NULL}};
  assert_expected_ast(tests, Parser_expression);
}

static void multiplicative_expressions(void** state) {
  // Test multiplicative expressions (including right-associativity)
  ParserTestFixture tests[] = {{"1*2/3", "(B (B (P 1), *, (P 2)), /, (P 3))"},
                               {"1*2%3", "(B (B (P 1), *, (P 2)), %, (P 3))"},
                               {NULL, NULL}};
  assert_expected_ast(tests, Parser_expression);
}

static void additive_expressions(void** state) {
  // Test additive_expressions (including right-associativity)
  ParserTestFixture tests[] = {{"a+b-c", "(B (B (P a), +, (P b)), -, (P c))"},
                               {NULL, NULL}};
  assert_expected_ast(tests, Parser_expression);
}

static void shift_expressions(void** state) {
  // Test shift expressions (including right-associativity)
  ParserTestFixture tests[] = {
      {"a>>b<<c", "(B (B (P a), >>, (P b)), <<, (P c))"}, {NULL, NULL}};
  assert_expected_ast(tests, Parser_expression);
}

static void relational_expressions(void** state) {
  // Test relational expresions
  ParserTestFixture tests[] = {{"a<b>c", "(B (B (P a), <, (P b)), >, (P c))"},
                               {NULL, NULL}};
  assert_expected_ast(tests, Parser_expression);
}

static void equality_expressions(void** state) {
  // Test equality expressions
  ParserTestFixture tests[] = {
      {"2==T!=P", "(B (B (P 2), ==, (P T)), !=, (P P))"}, {NULL, NULL}};

  assert_expected_ast(tests, Parser_expression);
}

static void logical_expressions(void** state) {
  // Test logical expressions (&, ^, |, &&, ||)
  ParserTestFixture tests[] = {{"1&2", "(B (P 1), &, (P 2))"},
                               {"a|b", "(B (P a), |, (P b))"},
                               {"a^b", "(B (P a), ^, (P b))"},
                               {"a&&b", "(B (P a), &&, (P b))"},
                               {"\"a\"||b", "(B (P \"a\"), ||, (P b))"},
                               {"1==2?1:3",
                                "(T (B (P 1), ==, (P 2)), (P 1), (P "
                                "3))"},
                               {NULL, NULL}};
  assert_expected_ast(tests, Parser_expression);
}

static void assignment_expressions(void** state) {
  ParserTestFixture tests[] = {{"1*=2", "(A (P 1), (B (P 1), *, (P 2)))"},
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

  assert_expected_ast(tests, Parser_expression);
}

static void expressions(void** state) {
  ParserTestFixture tests[] = {
      {"1+1,2+4", "(E (B (P 1), +, (P 1)), (B (P 2), +, (P 4)))"},
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
      cmocka_unit_test(shift_expressions),
      cmocka_unit_test(relational_expressions),
      cmocka_unit_test(equality_expressions),
      cmocka_unit_test(logical_expressions),
      cmocka_unit_test(assignment_expressions),
      cmocka_unit_test(expressions)};

  return cmocka_run_group_tests(tests, NULL, NULL);
}
