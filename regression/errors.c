/*
 * This C source file contains a number of errors which should
 * be picked up by acc's frontend. This includes:
 * 1. Scanner errors
 * 2. Parser errors
 * 3. Context-sensitive errors
 * 
 * This is ready by the regression test suite (see regression_test.py),
 * and compared against the acc's output.
 */

// !error SCANNER "Invalid character in input: '$'"
$

// !error SCANNER "Unterminated string literal"
char* a = "I forgot to add a closing quote...
// !error PARSER "Expected expression, got 'int'"
int a;

// !error PARSER "Invalid type"
static register int a;

// !error PARSER "Expected expression, got '/'"
int a = /;