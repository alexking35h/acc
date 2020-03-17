#ifndef __ERROR__
#define __ERROR__

#include <stdbool.h>

typedef enum ErrorType_t
{
    SCANNER,
    PARSER
} ErrorType;

typedef enum WarningType
{
    INVALID_INITIALIZER
} WarningType;

/* Error class */
typedef struct Error_t Error;

/*
 * Initialize Error.
 *
 * This function initializes an instance of the Error interface.
 * When finished, call the desctructor function (Error_destroy).
 *
 * Returns:
 *  pointer to allocated Error instance, or NULL on error.
 */
Error * Error_init(void);

/*
 * Report an error, with a line number and message
 *
 * Parameters:
 *  error - instance of Error class
 *  error_type
 *  line_number
 *  message (this can be NULL)
 */
void Error_report_error(Error *, ErrorType, int, const char *);

/*
 * Report a warning, with a line number and message
 *
 * Parameters:
 *  error - instance of Error class
 *  warning_type,
 *  line_number,
 *  message (this can be NULL)
 */
void Error_report_warning(Error *, WarningType, int, const char *);

/*
 * Check for errors
 *
 * Parameters:
 *  error - instance of Error class
 *
 * Returns:
 *  True/False if errors have been reported.
 */
bool Error_has_errors(Error *);

/*
 * Free up the Error class instance, and release all resources
 */
void Error_destroy(Error *);
 
#endif

