/*
 * Scanner / Lexer
 * 
 * The scanner generates a sequence of tokens from the input source code,
 * which are passed to the parser. Technically, the scanning is done
 * before Scanner_init() is called, and this is actually lexical analysis.
 * 
 * @TODO: s/Scanner/Lexer/g
 */

#ifndef __SCANNER__
#define __SCANNER__

#include "error.h"
#include "token.h"

/* Scanner instance */
typedef struct Scanner_t Scanner;

/*
 * Initialize Scanner
 *
 * This function initializes an instance of the Scanner.
 * When finished, call the destructor function (Scanner_destroy)
 *
 * Parameters:
 *  source - pointer to source code
 *
 * Returns:
 *  pointer to allocated Scanner instance, or NULL on error.
 */
Scanner *Scanner_init(char const *source, ErrorReporter *error_reporter);

/*
 * Allocate a new Token
 *
 * This is mostly used internally within the Scanner. However the
 * Parser may also wish to create new tokens for desugauring.
 */
Token *Scanner_create_token(Scanner *scanner);

/*
 * Get the next token
 *
 * This function generates the next token from the source file, including the
 * final EOF token. When finished, the function returns NULL.
 */
Token *Scanner_get_next(Scanner *scanner);

/*
 * Destroy instance of ACC's Scanner.
 */
void Scanner_destroy(Scanner *scanner);

/*
 * Get pointer to line position in the file.
 */
const char * Scanner_get_line(Scanner *scanner, int line);

#endif
