#include "test.h"

#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <cmocka.h>

#include "error.h"
#include "parser.h"
#include "pretty_print.h"
#include "scanner.h"
#include "symbol.h"
#include "token.h"
/*
 * Compare the expected textual AST representation for expr/decl/stmt
 * against 'expected'.
 */
_Bool test_compare_ast(const char *expected, ExprAstNode *expr, DeclAstNode *decl,
                       StmtAstNode *stmt)
{
    char ast_str[256] = "";
    if (expr)
    {
        pretty_print_expr(expr, ast_str, sizeof(ast_str));
    }
    else if (decl)
    {
        pretty_print_decl(decl, ast_str, sizeof(ast_str));
    }
    else if (stmt)
    {
        pretty_print_stmt(stmt, ast_str, sizeof(ast_str));
    }
    if (strlen(ast_str) != strlen(expected) || strcmp(ast_str, expected) != 0)
    {
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
_Bool test_parse_compare_ast(const char *source, const char *expected,
                             TestParserType test_type)
{
    Scanner *scanner = Scanner_init(source, MOCK_ERROR_REPORTER);
    Parser *parser = Parser_init(scanner, MOCK_ERROR_REPORTER);
    _Bool success;

    switch (test_type)
    {
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

    if (!Parser_at_end(parser)) {
        printf("FAIL. Expected 'END_OF_FILE'");
        success = false;
    }

    Parser_destroy(parser);
    Scanner_destroy(scanner);

    return success;
}

/*
 * For each <source input, expected> pair in test_set, parse the source C string,
 * and compare to against the expected textual AST representation.
 */
_Bool test_parse_compare_ast_set(AstTestSet test_set[], TestParserType test_type)
{
    for (; test_set->source; test_set++)
    {
        if (test_parse_compare_ast(test_set->source, test_set->expected, test_type) ==
            false)
        {
            return false;
        }
    }
    return true;
}

/*
 * Mock Error_report_error function
 */
void Error_report_error(ErrorReporter *error_reporter, ErrorType error_type, Position pos,
                        const char *msg)
{
    int line_number = pos.line;
    int line_position = pos.position;

    function_called();
    assert_true(error_reporter == MOCK_ERROR_REPORTER);
    check_expected(error_type);
    check_expected(line_number);
    check_expected(line_position);
    check_expected(msg);
}

/*
 * Helper function for declaring expected errors.
 */
void expect_report_error(ErrorType error_type, int expect_line, int expect_line_position,
                         char *expect_error_msg)
{
    expect_function_call(Error_report_error);
    expect_value(Error_report_error, error_type, error_type);
    expect_value(Error_report_error, line_number, expect_line);
    expect_value(Error_report_error, line_position, expect_line_position);
    expect_string(Error_report_error, msg, expect_error_msg);
}

/* Test symbol table Implementation */
SymbolTable *test_symbol_table;

static CType _int_type = {.type = TYPE_BASIC, .basic.type_specifier = TYPE_SIGNED_INT};
static CType _short_type = {.type = TYPE_BASIC,
                            .basic.type_specifier = TYPE_SIGNED_SHORT_INT};
static CType _char_type = {.type = TYPE_BASIC,
                           .basic.type_specifier = TYPE_UNSIGNED_CHAR};
static CType _ptr_type = {.type = TYPE_POINTER, .derived.type = &_int_type};
static CType _ptr_ptr_type = {.type = TYPE_POINTER, .derived.type = &_ptr_type};
static CType _function_type = {.type = TYPE_FUNCTION,
                               .derived.type = &_int_type,
                               .derived.params =
                                   &((ParameterListItem){NULL, &_int_type, NULL})};
static CType _short_ptr_type = {.type = TYPE_POINTER, .derived.type = &_short_type};
static CType _char_ptr_type = {.type = TYPE_POINTER, .derived.type = &_char_type};

Symbol *_int;
Symbol *_short_int;
Symbol *_char;
Symbol *_ptr;
Symbol *_ptr_ptr;
Symbol *_function;
Symbol *_short_ptr;
Symbol *_char_ptr;

void test_symbol_table_setup()
{
    test_symbol_table = symbol_table_create(NULL);
    _int = symbol_table_put(test_symbol_table, "_int", &_int_type);
    _short_int = symbol_table_put(test_symbol_table, "_short_int", &_short_type);
    _char = symbol_table_put(test_symbol_table, "_char", &_char_type);
    _ptr = symbol_table_put(test_symbol_table, "_ptr", &_ptr_type);
    _ptr_ptr = symbol_table_put(test_symbol_table, "_ptr_ptr", &_ptr_ptr_type);
    _function = symbol_table_put(test_symbol_table, "_function", &_function_type);
    _short_ptr = symbol_table_put(test_symbol_table, "_short_ptr", &_short_ptr_type);
    _char_ptr = symbol_table_put(test_symbol_table, "_char_ptr", &_char_ptr_type);
}

void test_symbol_table_teardown()
{
    free(test_symbol_table);
}