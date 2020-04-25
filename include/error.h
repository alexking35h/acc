#ifndef __ERROR__
#define __ERROR__

#include <stdbool.h>

typedef enum ErrorType_t { SCANNER, PARSER } ErrorType;

/*
 * Report an error, with a line number and message
 *
 * Parameters:
 *  error - instance of Error class
 *  error_type
 *  line_number
 *  message (this can be NULL)
 */
void Error_report_error(ErrorType, int, const char *);

#endif
