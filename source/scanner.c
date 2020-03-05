#ifndef __SCANNER__
#define __SCANNER__

#include <stdlib.h>
#include <stdio.h>

#include "token.h"
#include "scanner.h"

/* Scanner instance */
typedef struct Scanner_t
{
    // Source file
    char * source;
   
    // Current position in the input. 
    int current;
} Scanner;

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
Scanner * Scanner_init(char const * source_file)
{
    Scanner * scanner = malloc(sizeof(Scanner));   

    return scanner;
}

/*
 * Get the next token
 *
 * This function generates the next token from the source file, including the
 * final EOF token. When finished, the function returns NULL.
 */
Token * Scanner_get_next(Scanner * scanner)
{
    return NULL;
}

/*
 * Destroy instance of ACC's Scanner.
 */
void Scanner_destroy(Scanner * scanner)
{
    free(scanner);
}

int main()
{
    Scanner * scanner = Scanner_init("sdf");
    Scanner_destroy(scanner);
}

#endif
