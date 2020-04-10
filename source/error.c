#include "error.h"
#include <stdbool.h>
#include <stddef.h>

/*
 * Initialize Error.
 *
 * This function initializes an instance of the Error interface.
 * When finished, call the desctructor function (Error_destroy).
 *
 * Returns:
 *  pointer to allocated Error instance, or NULL on error.
 */
Error *Error_init(void) { return NULL; }

/*
 * Report an error, with a line number and message
 *
 * Parameters:
 *  error - instance of Error class
 *  error_type
 *  line_number
 *  message (this can be NULL)
 */
void Error_report_error(Error *error, ErrorType error_type, int line_number,
                        const char *msg) {
  *((int *)0) = 0;
}

/*
 * Report a warning, with a line number and message
 *
 * Parameters:
 *  error - instance of Error class
 *  warning_type,
 *  line_number,
 *  message (this can be NULL)
 */
void Error_report_warning(Error *error, WarningType warning_type,
                          int line_number, const char *msg) {}

/*
 * Check for errors
 *
 * Parameters:
 *  error - instance of Error class
 *
 * Returns:
 *  True/False if errors have been reported.
 */
bool Error_has_errors(Error *error) { return false; }

/*
 * Free up the Error class instance, and release all resources
 */
void Error_destroy(Error *error);
