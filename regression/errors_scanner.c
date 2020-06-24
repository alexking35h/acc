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


