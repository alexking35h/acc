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
  DeclAstNode* ast_node = Parser_declaration(parser);

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

static void primitive_declaration(void** state) {
  Fixture tests[] = { {"int c;", "(D [int], c)"}, {NULL, NULL};
  assert_expected_ast(tests);
}

int main(void) {
  const struct CMUnitTest tests[] = {cmocka_unit_test(primitive_declaration)};

  return cmocka_run_group_tests(tests, NULL, NULL);
}
