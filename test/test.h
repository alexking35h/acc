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
_Bool test_ast_compare_expr(const char* source, const char* expected);

/*
 * Check if the string representation for a declaration source input matches
 * the expected AST. (Assuming no errors occur.)
 */
_Bool test_ast_compare_decl(const char* source, const char* expected);

/*
 * Check if the string representation for a statement source input matches
 * the expected AST. (Assuming no errors occur.)
 */
_Bool test_ast_compare_stmt(const char* source, const char* expected);

/*
 * For each <source input, expected AST> pair in the test fixture, verify
 * the expression AST generated by the parser.
 */
void assert_expected_ast_expr(AstTestFixture* fixture);

/*
 * For each <source input, expected AST> pair in the test fixture, verify
 * the declaration AST generated by the parser.
 */
void assert_expected_ast_decl(AstTestFixture* fixture);

/*
 * For each <source input, expected AST> pair in the test fixture, verify
 * the statement AST generated by the parser.
 */
void assert_expected_ast_stmt(AstTestFixture* fixture);

/*
 * Helper function for declaring expected errors.
 */
void expect_report_error(int expect_line, char* expect_err_str);

#endif
