#include "error.h"
#include <stdlib.h>
#include <string.h>

void Error_report_error(ErrorReporter *, ErrorType, int, int, const char *, const char *)
    __attribute__((weak));

typedef struct ErrorReport {
    ErrorType type;
    int line_number;
    int line_position;
    char * title;
    char * description;   

    struct ErrorReport * next;
} ErrorReport;

typedef struct ErrorReporter {
    ErrorReport * head;
    ErrorReport * iterate_position;
} ErrorReporter;

ErrorReporter * Error_init() {
    return calloc(1, sizeof(ErrorReport));
}

void Error_destroy(ErrorReporter * error_reporter) {
    for(ErrorReport *e = error_reporter->head;e;) {
        ErrorReport *next = e->next;
        free(e);
        e = next;
    }
    free(error_reporter);
}

void Error_report_error(
    ErrorReporter *error_reporter,
    ErrorType type,
    int line_number,
    int line_position,
    const char * title,
    const char * description)
{
    // Find the right position in the list. Ordered as follows:
    // 1. By line-number ascending.
    // 2. By line-position ascending.
    // 3. By type: SCANNER, PARSER, ANALYSIS.
    ErrorReport ** prev = &error_reporter->head;
    while(true) {
        if(*prev == NULL) {
            // end of list.
            break;
        }

        if((*prev)->line_number < line_number) {}
        else if ((*prev)->line_position < line_position) {}
        else if ((*prev)->type < type) {}
        else {
             break;
        }
        prev = &((*prev)->next);
    }

    // Insert the new node.
    ErrorReport* next = *prev;
    *prev = calloc(1, sizeof(ErrorReport));
    (*prev)->next = next;

    // Store error information.
    (*prev)->line_number = line_number;
    (*prev)->line_position = line_position;
    (*prev)->type = type;

    (*prev)->title = malloc(strlen(title));
    strcpy((*prev)->title, title);

    if(description) {
        (*prev)->description = malloc(strlen(description));
        strcpy((*prev)->description, description);
    } 
}

int Error_get_errors(
    ErrorReporter * error_reporter,
    ErrorType *type,
    int * line_number,
    int * line_position,
    char ** title,
    char ** description,
    _Bool beginning
) {
    if(beginning)
        error_reporter->iterate_position = error_reporter->head;
    
    if(!error_reporter->iterate_position)
        return 0;
    
    ErrorReport *n = error_reporter->iterate_position;

    *type = n->type;
    *line_number = n->line_number;
    *line_position = n->line_position;
    *title = n->title;
    *description = n->description;

    error_reporter->iterate_position = n->next;
    return 1;
}

int Error_has_errors(ErrorReporter * error_reporter) {
    return error_reporter->head != NULL;
}