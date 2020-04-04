#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include <cmocka.h>

#include <stdio.h>
#include <string.h>

#include "parser.h"
#include "pretty_print.h"
#include "scanner.h"
#include "test.h"

/*
 * Check if the string representation for an expression source input matches
 * the expected AST. (Assuming no errors occur.)
 */
bool test_ast_compare_expr(const char* source, const char* expected) {
  // Generate Scanner and Parser objects.
  Scanner* scanner = Scanner_init(source, NULL);
  Parser* parser = Parser_init(scanner, NULL);

  // Generate the AST
  ExprAstNode* ast_node = Parser_expression(parser);

  char generated_ast[256] = "";
  pretty_print_expr(ast_node, generated_ast, sizeof(generated_ast));

  bool matches = (strlen(generated_ast) == strlen(expected) &&
                  strcmp(generated_ast, expected) == 0);

  if (!matches) {
    printf("%s FAIL. Expected '%s', got '%s'\n", __FUNCTION__, expected,
           generated_ast);
  }

  Parser_destroy(parser);
  Scanner_destroy(scanner);

  return matches;
}

/*
 * Check if the string representation for a declaration source input matches
 * the expected AST. (Assuming no errors occur.)
 */
bool test_ast_compare_decl(const char* source, const char* expected) {
  // Generate Scanner and Parser objects.
  Scanner* scanner = Scanner_init(source, NULL);
  Parser* parser = Parser_init(scanner, NULL);

  // Generate the AST
  DeclAstNode* ast_node = Parser_declaration(parser);

  char generated_ast[256] = "";
  pretty_print_decl(ast_node, generated_ast, sizeof(generated_ast));

  bool matches = (strlen(generated_ast) == strlen(expected) &&
                  strcmp(generated_ast, expected) == 0);

  if (!matches) {
    printf("%s FAIL. Expected '%s', got '%s'\n", __FUNCTION__, expected,
           generated_ast);
  }

  Parser_destroy(parser);
  Scanner_destroy(scanner);

  return matches;
}

/*
 * Assert the string representation for the declaration AST matches the expected
 * AST for a given C source input.
 */
void assert_expected_ast_expr(AstTestFixture* fixture) {
  for (; fixture->source; fixture++) {
    assert_true(test_ast_compare_expr(fixture->source, fixture->expected));
  }
}

/*
 * Assert the string representation for the declaration AST matches the expected
 * AST for a given C source input.
 */
void assert_expected_ast_decl(AstTestFixture* fixture) {
  for (; fixture->source; fixture++) {
    assert_true(test_ast_compare_decl(fixture->source, fixture->expected));
  }
}
