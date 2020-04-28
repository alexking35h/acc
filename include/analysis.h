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

#endif