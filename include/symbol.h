/*
 * Symbol tables in acc map identifiers to type/location information
 * for variables and functions. During context-sensitive analysis,
 * symbol tables are generated for each block.
 */
#ifndef __SYMBOL__
#define __SYMBOL__

#include <stddef.h>

#include "ctype.h"

/*
 * Symbol table entry for a single symbol.
 * 
 * This includes:
 * - type information
 * - variable/function name
 * - size
 * - offset
 */
typedef struct Symbol_t {
  char* name;
  CType* type;
  int size; // (in bytes)
  int offset; // (in bytes)
} Symbol;

typedef struct SymbolTable_t SymbolTable;

/*
 * Create a new symbol table.
 * 
 * 'parent' is the parent scope. This is 'NULL' for global-scope.
 */
SymbolTable* symbol_table_create(SymbolTable* parent);

/*
 * Define a entry in a symbol table.
 */
void symbol_table_put(SymbolTable* table, char* name, CType* type);

/*
 * Retrieve a symbol table entry within this scope.
 * 
 * 'search_parent' instructs this function to recursively 
 * search ancestors' symbol tables also.
 */
Symbol* symbol_table_get(SymbolTable* table, char* name, bool search_parent);

#endif