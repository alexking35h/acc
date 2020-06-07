/*
 * Error-handling interface.
 *
 * Errors are reported during scanning, parsing, and code-gen,
 * by calling Error_report_error. This function is implemented
 * in acc.c, and is responsible for making sure we displaying
 * error messages, and making we only compile if the input is
 * error-free.
 */
#ifndef __ERROR__
#define __ERROR__

#include <stdbool.h>

typedef enum ErrorType_t
{
    SCANNER,
    PARSER,
    ANALYSIS
} ErrorType;

typedef struct ErrorReporter ErrorReporter;

ErrorReporter * Error_init();
void Error_destroy(ErrorReporter *);
int Error_get_errors(
    ErrorReporter * error_reporter,
    ErrorType *type,
    int * line_number,
    int * line_position,
    char ** title,
    char ** description,
    _Bool beginning
);
int Error_has_errors(ErrorReporter *);

/*
 * Report an error, with a line number and message
 *
 * Parameters:
 *  error - instance of Error class
 *  error_type
 *  line_number
 *  line_position
 *  title
 *  description
 */
void Error_report_error(ErrorReporter *, ErrorType, int, int, const char *, const char *);

#endif
