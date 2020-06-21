#ifndef __UTIL_H__
#define __UTIL_H__

#define STR_CONCAT(...) str_concat((char *[]){__VA_ARGS__, NULL})

/*
 * Concatenate a sequence of null-terminated strings into a single string.
 * The final pointer in the list must be NULLL.
 * 
 * The returned string is allocated from the heap.
 */
char * str_concat(char**);

#endif