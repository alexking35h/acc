#ifndef __SCANNER__
#define __SCANNER__

#include "token.h"

/* Scanner instance */
typedef struct Scanner_t Scanner;

/*
 * Initialize Scanner
 *
 * This function initializes an instance of the Scanner for a given source 
 * file. When finished, call the destructor function (Scanner_destroy)
 *
 * Parameters:
 *  source_file - path to source file
 *
 * Returns:
 *  pointer to allocated Scanner instance, or NULL on error.
 */
Scanner * Scanner_init(char const * source_file);

/*
 * Get the next token
 *
 * This function generates the next token from the source file, including the
 * final EOF token. When finished, the function returns NULL.
 */
Token * Scanner_get_next(Scanner * scanner);

/*
 * Destroy instance of ACC's Scanner.
 */
void Scanner_destroy(Scanner * scanner);

#endif

