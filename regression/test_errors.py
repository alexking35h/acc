import functools
import pytest
import functional

COMPILERS = [
    functional.GccCompiler,
    functools.partial(functional.AccCheckOnlyCompiler, "../build/acc"),
]


@pytest.fixture(params=COMPILERS)
def cc(request):
    return request.param(None)


def test_scanner_invalid_character(cc):
    err = functional.CompilerError("SCANNER", 3, "Invalid character in input: '$'")
    cc.program("int a;\n\n$\n\n", [err])


def test_scanner_unterminated_string(cc):
    err = functional.CompilerError("SCANNER", 1, "Unterminated string literal")
    cc.program('"Oops, forgot to close this...\n', [err])

def test_parser_invalid_type(cc):
    err = functional.CompilerError("PARSER", 1, "Invalid type")

    # Multiple specifiers.
    cc.program("int char a;", [err])

    # Multiple signedness specifiers
    cc.program("signed unsigned b", [err])

    # Multiple size specifiers 
    cc.program("short long int b;", [err])

    # Multiple storage class specifiers
    cc.program("extern static int q;", [err])

    # Invalid char type.
    cc.program("short char a;", [err])

def test_parser_invalid_function_type(cc):
    err = functional.CompilerError("PARSER", 1, "Functions cannot return functions (try Python?)")
    cc.program("int a()(){{}}", [err])
    cc.program("int a()();", [err])

    err = functional.CompilerError("PARSER", 1, "Functions cannot return arrays (try Python?)")
    cc.program("int a()[2];", [err])
    cc.program("int a()[3]{{}}", [err])

def test_parser_invalid_type_name(cc):
    err = functional.CompilerError("PARSER", 1, "Type names must not have an identifier")
    cc.program("int a = (int b)12;", [err])

def test_parser_invalid_declaration(cc):
    err = functional.CompilerError("PARSER", 1, "Missing identifier in declaration")
    cc.program("unsigned short int = 4;", [err])

def test_parser_expected_expression(cc):
    err = functional.CompilerError("PARSER", 1, "Expected expression, got 'int'")
    cc.program("int q = int;", [err])

def test_parser_error(cc):
    parser_errors = [
        ("return return 23;", "Expected expression, got 'return'"),
        ("int a = (12 + 3;", "Expecting ')', got ';'"),
        ("int q = 12 + 3 / % ;", "Expected expression, got '%'"),
        ("while(a{ { return 1;}", "Expecting ')', got '{'"),
        ("unsigned char * a = &!;", "Expected expression, got ';'")
    ]

    for source, error in parser_errors:
        cc.body(source, [functional.CompilerError("PARSER", 3, error)])