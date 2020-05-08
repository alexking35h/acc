#include "test.h"

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>


#include <cmocka.h>


#include "parser.h"
#include "pretty_print.h"
#include "scanner.h"
#include "token.h"
#include "error.h"

/*
 * Compare the expected textual AST representation for expr/decl/stmt
 * against 'expected'.
 */
_Bool test_compare_ast(const char* expected, ExprAstNode* expr, DeclAstNode* decl, StmtAstNode* stmt) {
  char ast_str[256] = "";
  if(expr) {
    pretty_print_expr(expr, ast_str, sizeof(ast_str));
  } else if (decl) {
    pretty_print_decl(decl, ast_str, sizeof(ast_str));
  } else if (stmt) {
    pretty_print_stmt(stmt, ast_str, sizeof(ast_str));
  }
  if (strlen(ast_str) != strlen(expected) || strcmp(ast_str, expected) != 0) {
    printf("FAIL. Expected '%s', got '%s'\n", expected, ast_str);
    return false;
  }
  return true;
}

/*
 * Parse the 'source' C string, and compare to the expected textual AST
 * representation 'expected'. TestParserType selects the Parser function:
 * - TEST_EXPR: Parser_expression()
 * - TEST_DECL: Parser_declaration()
 * - TEST_STMT: Parser_statement()
 */
_Bool test_parse_compare_ast(const char* source, const char* expected, TestParserType test_type) {
  Scanner* scanner = Scanner_init(source);
  Parser* parser = Parser_init(scanner);
  _Bool success;

  switch(test_type) {
    case TEST_EXPR:
      success = test_compare_ast_expr(expected, Parser_expression(parser));
      break;
    case TEST_DECL:
      success = test_compare_ast_decl(expected, Parser_translation_unit(parser));
      break;
    case TEST_STMT:
      success = test_compare_ast_stmt(expected, Parser_compound_statement(parser));
      break;
  }

  if (Parser_peek_next_token(parser)->type != END_OF_FILE) {
    TokenType next_token = Parser_peek_next_token(parser)->type;
    printf("FAIL. Expected 'END_OF_FILE', got '%s'\n", Token_str(next_token));
  }

  Parser_destroy(parser);
  Scanner_destroy(scanner);

  return success;
}

/*
 * For each <source input, expected> pair in test_set, parse the source C string,
 * and compare to against the expected textual AST representation.
 */
_Bool test_parse_compare_ast_set(AstTestSet test_set[], TestParserType test_type) {
  for(;test_set->source; test_set++) {
    if(test_parse_compare_ast(test_set->source, test_set->expected, test_type) == false) {
      return false;
    }
  }
  return true;
}

/*
 * Mock Error_report_error function
 */
void Error_report_error(ErrorType error_type, int line_number,
                        const char* error_string) {
  function_called();
  check_expected(error_type);
  check_expected(line_number);
  check_expected(error_string);
}

/*
 * Helper function for declaring expected errors.
 */
void expect_report_error(ErrorType error_type, int expect_line, char* expect_err_str) {
  expect_function_call(Error_report_error);
  expect_value(Error_report_error, error_type, error_type);
  expect_value(Error_report_error, line_number, expect_line);
  expect_string(Error_report_error, error_string, expect_err_str);
}

/* 
 * Mock symbol table functions
 */
SymbolTable* __wrap_symbol_table_create(SymbolTable* parent){
    check_expected(parent);
    return (SymbolTable*)mock();
}
Symbol* __wrap_symbol_table_put(SymbolTable* tab, char* name, CType* type) {
    if(!strcmp(name, "ignoreme")) return NULL;
    check_expected(tab);
    check_expected(name);
    return (Symbol*)mock();
}
Symbol* __wrap_symbol_table_get(SymbolTable* tab, char* name, bool search_parent) {
    if(!strcmp(name, "ignoreme")) return NULL;
    check_expected(tab);
    check_expected(name);
    check_expected(search_parent);
    return (Symbol*)mock();
}

/* 
 * Helper functions for checking mock function parameters and setting return values
 */
void expect_symbol_create(SymbolTable* parent, SymbolTable* ret) {
    expect_value(__wrap_symbol_table_create, parent, parent);
    will_return(__wrap_symbol_table_create, ret);
}
void expect_symbol_put(SymbolTable* tab, char* name, Symbol* ret) {
    expect_value(__wrap_symbol_table_put, tab, tab);
    expect_string(__wrap_symbol_table_put, name, name);
    will_return(__wrap_symbol_table_put, ret);
}
void expect_symbol_get(SymbolTable* tab, char* name, bool search_parent, Symbol* ret) {
    expect_value(__wrap_symbol_table_get, tab, tab);
    expect_string(__wrap_symbol_table_get, name, name);
    expect_value(__wrap_symbol_table_get, search_parent, search_parent);
    will_return(__wrap_symbol_table_get, ret);
}