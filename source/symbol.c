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
  // This is stored as a dynamically allocated array in memory.
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
void symbol_table_put(SymbolTable* tab, char* name, CType* type) {
    Symbol* sym = NULL;
    if(!tab->symbols_list) {
      tab->symbols_list = sym = malloc(sizeof(Symbol));
    } else {
      int sz = sizeof(Symbol) * (tab->symbols_count + 1);
      tab->symbols_list = realloc(tab->symbols_list, sz);
      sym = &tab->symbols_list[tab->symbols_count];
    }

    tab->symbols_count++;
    sym->name = name;
    sym->type = type;
}

/*
 * Retrieve a symbol table entry within this scope.
 * 
 * 'search_parent' instructs this function to recursively 
 * search ancestors' symbol tables also.
 */
Symbol* symbol_table_get(SymbolTable* tab, char* name, bool search_parent){
  for(int i = 0;i < tab->symbols_count;++i) {
    if(strcmp(name, tab->symbols_list[i].name) == 0)
      return &tab->symbols_list[i];
  }
  if(search_parent == false || tab->parent == NULL)
    return NULL;
  
  return symbol_table_get(tab->parent, name, true);
}