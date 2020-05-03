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
void analysis_ast_walk(DeclAstNode*, ExprAstNode*, StmtAstNode*, SymbolTable*);

#define analysis_ast_walk_decl(decl, tab) analysis_ast_walk(decl, NULL, NULL, tab)
#define analysis_ast_walk_expr(expr, tab) analysis_ast_walk(NULL, expr, NULL, tab)
#define analysis_ast_walk_stmt(stmt, tab) analysis_ast_walk(NULL, NULL, stmt, tab)


#endif