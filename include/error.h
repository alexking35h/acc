/*
 * Error-handling interface.
 *
 * Errors are reported during scanning, parsing, and analysis
 * by calling Error_reporter_error. Errors are collected by the
 * ErrorReporter class, which collects for each error:
 *  - Type
 *  - Line number
 *  - Line position
 *  - Title (brief description of the error)
 *  - Description
 *
 * Errors are retrieved from the ErrorReporter class using the
 * Error_get_errors method. This can be called after scanning/parsing/analysis
 * to review all reported errors.
 */
#ifndef __ERROR__
#define __ERROR__

#include <stdbool.h>

#include "token.h"

typedef enum ErrorType_t
{
    SCANNER,
    PARSER,
    ANALYSIS
} ErrorType;

/*
 * Forward definition of the ErrorReporter class.
 */
typedef struct ErrorReporter ErrorReporter;

/*
 * Initialize instance of ErrorReporter.
 *
 * This should be later destroyed with Error_destroy()
 */
ErrorReporter *Error_init();

/*
 * Destroy/release an ErrorReporter instance. This frees
 * all allocated memory associated with the class.
 */
void Error_destroy(ErrorReporter *);

/*
 * Check if any errors were reported
 */
int Error_has_errors(ErrorReporter *);

/*
 * Iterate through errors reported to the ErrorReporter instance.
 *
 * This function iterates through the reported errors, and sets the error
 * attributes (type, line, position, title, description) for each error.
 * To start from the beginning of the reported errors list, set beginning = true.
 *
 * Returns 0 if there are no more errors left.
 */
int Error_get_errors(ErrorReporter *error_reporter, ErrorType *type, int *line_number,
                     int *line_position, char **msg, _Bool beginning);

/*
 * Report an error, with a line number and message.
 * title and description arguments will be copied.
 *
 * Parameters:
 *  ErrorReporter * error - Error Reporter class
 *  ErrorType error_type
 *  int line_number
 *  int line_position
 *  char * msg
 */
void Error_report_error(ErrorReporter *, ErrorType, Position, const char *);

#endif
