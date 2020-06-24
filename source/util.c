#include <stdlib.h>
#include <string.h>
#include <util.h>

/*
 * Concatenate a sequence of null-terminated strings into a single string.
 * The final pointer in the list must be NULLL.
 *
 * The returned string is allocated from the heap.
 */
char *str_concat(char **source_str)
{
    // Find out how much space is needed for the total string.
    int len = 0;
    for (char **str = source_str; *str != NULL; str++)
    {
        len += strlen(*str);
    }

    char *new_str = calloc(len + 1, sizeof(char));
    for (char **str = source_str; *str != NULL; str++)
    {
        strcat(strchr(new_str, '\0'), *str);
    }
    return new_str;
}