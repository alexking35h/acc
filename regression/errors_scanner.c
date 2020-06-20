/*
 * This C source files contains a number of scanner/lexer (microsyntactic) errors
 * which should be picked up by acc's frontend.
 * 
 * This is read by the regression test suite (see regression_test.py), and compared
 * with acc's output.
 */

// !error SCANNER "Invalid character in input: '$'"
$

// !error SCANNER "Invalid character in input: '@'"
@

// !error SCANNER "Unterminated string literal"
"I forgot to add a closing quote...

// !error PARSER "Invalid type"
static register int a;

// !error PARSER "Expected expression, got '/'"
int a = /;

void undeclared_identifier() {
    // !error ANALYSIS "Undeclared identifier 'q'"
    return q;
}

void invalid_lvalue() {
    // !error ANALYSIS "Invalid lvalue"
    3 = 2;
}

void invalid_pointer_dereference() {
    char *a;
    // !error ANALYSIS "Invalid Pointer dereference"
    **a = 12;
}

void invalid_assignment() {
    char *b;
    // !error ANALYSIS "Incompatible assignment. Cannot assign type 'int signed' to type 'pointer to char'"
    b = 12;
}