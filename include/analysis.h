/*
 * Context-sensitive Analysis 
 * 
 * This phase walks the AST and does context-sensitive/semantic
 * analysis to look for errors; generate symbol tables; and 
 * get ready for code generation.
 */
#ifndef __ANALYSIS__
#define __ANALYSIS__

#include "ast.h"
#include "symbol.h"
#include "parser.h"

/*
 * Walk the AST
 */
void analysis_ast_walk(DeclAstNode*, SymbolTable**);

/*
 * Functions are declared globally for AST node
 * subtypes to make testing easier. These functions should
 * not be used outside of unit tests.
 */
void analysis_ast_walk_decl(DeclAstNode*, SymbolTable*);
void analysis_ast_walk_expr(ExprAstNode*, SymbolTable*);
void analysis_ast_walk_stmt(ExprAstNode*, SymbolTable*);

#endif