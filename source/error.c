#include "error.h"
#include <stdlib.h>
#include <string.h>

void Error_report_error(ErrorReporter *, ErrorType, Position, const char *)
    __attribute__((weak));

typedef struct ErrorReport
{
    ErrorType type;
    int line_number;
    int line_position;
    char *msg;

    struct ErrorReport *next;
} ErrorReport;

typedef struct ErrorReporter
{
    ErrorReport *head;
    ErrorReport *iterate_position;
} ErrorReporter;

ErrorReporter *Error_init()
{
    return calloc(1, sizeof(ErrorReport));
}

void Error_destroy(ErrorReporter *error_reporter)
{
    for (ErrorReport *e = error_reporter->head; e;)
    {
        ErrorReport *next = e->next;
        free(e);
        e = next;
    }
    free(error_reporter);
}

void Error_report_error(ErrorReporter *error_reporter, ErrorType type, Position position, const char *msg)
{
    // Find the right position in the list. Ordered as follows:
    // 1. By line-number ascending.
    // 2. By line-position ascending.
    // 3. By type: SCANNER, PARSER, ANALYSIS.
    ErrorReport **prev = &error_reporter->head;
    for (; *prev; prev = &((*prev)->next))
    {

        if (((*prev)->line_number < position.line) ||
            ((*prev)->line_position < position.position) || ((*prev)->type < type))
        {
            continue;
        }
        else
        {
            break;
        }
    }

    // Insert the new node.
    ErrorReport *next = *prev;
    *prev = calloc(1, sizeof(ErrorReport));
    (*prev)->next = next;

    // Store error information.
    (*prev)->line_number = position.line;
    (*prev)->line_position = position.position;
    (*prev)->type = type;
    (*prev)->msg = malloc(strlen(msg));
    strcpy((*prev)->msg, msg);
}

int Error_get_errors(ErrorReporter *error_reporter, ErrorType *type, int *line_number,
                     int *line_position, char **msg, _Bool beginning)
{
    if (beginning)
        error_reporter->iterate_position = error_reporter->head;

    if (!error_reporter->iterate_position)
        return 0;

    ErrorReport *n = error_reporter->iterate_position;

    *type = n->type;
    *line_number = n->line_number;
    *line_position = n->line_position;
    *msg = n->msg;

    error_reporter->iterate_position = n->next;
    return 1;
}

int Error_has_errors(ErrorReporter *error_reporter)
{
    return error_reporter->head != NULL;
}