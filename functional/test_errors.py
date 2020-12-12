import functools
import pytest
import os

from acctools import compilers

ACC_PATH=os.environ.get("ACC_PATH", os.path.join(os.path.dirname(__file__), "../build/acc"))

COMPILERS = [
    compilers.GccCompiler,
    functools.partial(compilers.AccCheckOnlyCompiler, ACC_PATH)
]


@pytest.fixture(params=COMPILERS)
def cc(request):
    return request.param(None)


def test_scanner_invalid_character(cc):
    err = compilers.CompilerError("SCANNER", 3, "Invalid character in input: '$'")
    cc.program("int a;\n\n$\n\n", [err])


def test_scanner_unterminated_string(cc):
    err = compilers.CompilerError("SCANNER", 1, "Unterminated string literal")
    cc.program('"Oops, forgot to close this...\n', [err])

def test_parser_invalid_type(cc):
    err = compilers.CompilerError("PARSER", 1, "Invalid type")

    # Multiple specifiers.
    cc.program("int char a;", [err])

    # Multiple signedness specifiers
    cc.program("signed unsigned b", [err])

    # Multiple storage class specifiers
    cc.program("extern static int q;", [err])

    # Invalid char type.
    cc.program("short char a;", [err])

def test_parser_invalid_function_type(cc):
    err = compilers.CompilerError("PARSER", 1, "Functions cannot return functions (try Python?)")
    cc.program("int a()(){{}}", [err])
    cc.program("int a()();", [err])

    err = compilers.CompilerError("PARSER", 1, "Functions cannot return arrays (try Python?)")
    cc.program("int a()[2];", [err])
    cc.program("int a()[3]{{}}", [err])

def test_parser_invalid_type_name(cc):
    err = compilers.CompilerError("PARSER", 1, "Type names must not have an identifier")
    cc.program("int a = (int b)12;", [err])

def test_parser_invalid_declaration(cc):
    err = compilers.CompilerError("PARSER", 1, "Missing identifier in declaration")
    cc.program("unsigned short int = 4;", [err])

def test_parser_expected_expression(cc):
    err = compilers.CompilerError("PARSER", 1, "Expected expression, got 'int'")
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
        cc.body(source, [compilers.CompilerError("PARSER", 3, error)])
    
def test_analysis_undeclared_identifier(cc):
    err = compilers.CompilerError("ANALYSIS", 3, "Undeclared identifier 'q'")
    cc.expression("q", [err])

def test_analysis_previously_declared_identifier(cc):
    err = compilers.CompilerError("ANALYSIS", 3, "Previously declared identifier 'a'")
    cc.body("int a;int a;", [err])

def test_analysis_invalid_lvalue(cc):
    err = compilers.CompilerError("ANALYSIS", 3, "Invalid lvalue")
    cc.body("3 == 3 = 2;", [err])
    cc.body("3 = 3;", [err])

def test_analysis_invalid_pointer_dereference(cc):
    err = compilers.CompilerError("ANALYSIS", 3, "Invalid Pointer dereference")
    cc.body("char *a;**a = 12;", [err])

def test_invalid_assignment(cc):
    err_msg = (
        "Incompatible assignment. Cannot assign type 'int signed' to "
        "type 'pointer to char'"
    )
    err = compilers.CompilerError("ANALYSIS", 3, err_msg)
    cc.body("char * b;b = 12;", [err])

def test_incompatible_function_declaration(cc):
    err_msg = "function definition does not match prior declaration"
    err = compilers.CompilerError("ANALYSIS", 1, err_msg)
    cc.program("int f(char a, int b);int f(char a, int * b){}", [err])

@pytest.mark.skip("Not implemented yet")
def test_missing_function_definition(cc):
    err_msg = "Missing function definition 'missing'"
    err = compilers.CompilerError("ANALYSIS", 1, err_msg)
    cc.program("int a();int main(){return a();}", [err])