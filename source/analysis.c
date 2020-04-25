#include "analysis.h"
#include "ast.h"
#include "symbol.h"

/*
 * Walk the AST.
 */
void analysis_ast_walk(DeclAstNode* root, SymbolTable** global) {
    *global = symbol_table_create(NULL);
}