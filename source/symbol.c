#include <stdlib.h>
#include <string.h>

#include "symbol.h"
#include "ctype.h"

/*
 * Symbol Table struct. This includes:
 * - set of Symbol's defined within this scope
 * - pointer to the parent scope/symbol table.
 */
struct SymbolTable_t {
  struct SymbolTable_t * parent;

  // List of symbols within this symbol table.
  struct Symbol_t * symbols_list;
  int symbols_count;
};

/*
 * Create a new symbol table.
 * 
 * 'parent' is the parent scope. This is 'NULL' for global-scope.
 */
SymbolTable* symbol_table_create(SymbolTable* parent) {
  SymbolTable* table = calloc(1, sizeof(SymbolTable));
  table->parent = parent;
  table->symbols_list = NULL;
  table->symbols_count = 0;
  return table;
}

/*
 * Define a entry in a symbol table.
 */
Symbol* symbol_table_put(SymbolTable* tab, char* name, CType* type) {
    Symbol** ptr = &tab->symbols_list;
    for(;*ptr != NULL;ptr = &(**ptr).next);

    *ptr = calloc(1, sizeof(Symbol));
    (*ptr)->name = name;
    (*ptr)->type = type;

    return *ptr;
}

/*
 * Retrieve a symbol table entry within this scope.
 * 
 * 'search_parent' instructs this function to recursively 
 * search ancestors' symbol tables also.
 */
Symbol* symbol_table_get(SymbolTable* tab, char* name, bool search_parent){
  for(Symbol* sym = tab->symbols_list; sym != NULL; sym = sym->next) {
    if(strcmp(name, sym->name) == 0) {
      return sym;
    }
  }
  if(search_parent == false || tab->parent == NULL)
    return NULL;
  
  return symbol_table_get(tab->parent, name, true);
}