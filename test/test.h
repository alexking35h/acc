#ifndef __TEST__
#define __TEST__

typedef struct AstTestFixture_t {
  char* source;
  char* expected;
} AstTestFixture;

/*
 * Check if the string representation for an expression source input matches
 * the expected AST. (Assuming no errors occur.)
 */
bool test_ast_compare_expr(const char* source, const char* expected);

/*
 * Check if the string representation for a declaration source input matches
 * the expected AST. (Assuming no errors occur.)
 */
bool test_ast_compare_decl(const char* source, const char* expected);

/*
 * Assert the string representation for the declaration AST matches the expected
 * AST for a given C source input.
 */
void assert_expected_ast_expr(AstTestFixture* fixture);

/*
 * Assert the string representation for the declaration AST matches the expected
 * AST for a given C source input.
 */
void assert_expected_ast_decl(AstTestFixture* fixture);

#endif
