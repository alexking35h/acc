/*
 * This C source files contains a number of errors which should be picked up during
 * context-sensitive (semantic) analysis.
 * 
 * This is read by the regression test suite (see regression_test.py), and compared
 * with acc's output.
 */

int a;

// !error ANALYSIS "Previously declared identifier 'a'"
int a;

void undeclared_identifier() {
    // !error ANALYSIS "Undeclared identifier 'q'"
    return q;
}

void previously_declared_identifier() {
    int a; // This is fine.

    // !error ANALYSIS "Previously declared identifier 'a'"
    char ** a;
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