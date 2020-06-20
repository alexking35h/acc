/*
 * This C source files contains a number of grammar errors,
 * which should be picked up by acc's frontend.
 * 
 * This is read by the regression test suite (see regression_test.py), and compared
 * with acc's output.
 */

// !error PARSER "Invalid type"
char int a;

// !error PARSER "Invalid type"
long short int b;

// !error PARSER "Invalid type"
static register int c;

// !error PARSER "Expected expression, got '/'"
int a = /;

// Note: we need two semicolons, because acc will try to synchronise on the next one.
// !error PARSER "Missing identifier in declaration"
int int;;

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