#ifndef __TEST__
#define __TEST__

#include "ast.h"
#include "error.h"
#include "symbol.h"

typedef enum {
  TEST_EXPR,
  TEST_DECL,
  TEST_STMT
} TestParserType;

typedef struct AstTestSet {
  char* source;
  char* expected;
} AstTestSet;

#define test_compare_ast_expr(expected, expr) test_compare_ast(expected, expr, NULL, NULL)
#define test_compare_ast_decl(expected, decl) test_compare_ast(expected, NULL, decl, NULL)
#define test_compare_ast_stmt(expected, stmt) test_compare_ast(expected, NULL, NULL, stmt)

/*
 * Compare the expected textual AST representation for expr/decl/stmt
 * against 'expected'.
 */
_Bool test_compare_ast(const char* expected, ExprAstNode* expr, DeclAstNode* decl, StmtAstNode* stmt);

/*
 * Parse the 'source' C string, and compare to the expected textual AST
 * representation 'expected'. TestParserType selects the Parser function:
 * - TEST_EXPR: Parser_expression()
 * - TEST_DECL: Parser_declaration()
 * - TEST_STMT: Parser_statement()
 */
_Bool test_parse_compare_ast(const char* source, const char* expected, TestParserType test_type);

/*
 * For each <source input, expected> pair in test_set, parse the source C string,
 * and compare to against the expected textual AST representation.
 */
_Bool test_parse_compare_ast_set(AstTestSet test_set[], TestParserType);


/*
 * Helper function for declaring expected errors.
 */
void expect_report_error(ErrorType error_type, int expect_line, char* expect_err_str);

/*
 * Test symbol table used in:
 * - test_analysis_type_checking
 * - test_analysis_conversions
 *
 * Setup/teardown with:
 * - symbol_table_setup()
 * - symbol_table_teardown()
 */
Symbol *_int;
Symbol *_long_int;
Symbol *_char;
Symbol *_ptr;
Symbol *_ptr_ptr;
Symbol *_function;
extern SymbolTable *test_symbol_table;
void test_symbol_table_setup();
void test_symbol_table_teardown();

#endif
