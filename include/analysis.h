/*
 * Context-sensitive Analysis 
 * 
 * This phase walks the AST generated by the parser, in order to prepare the AST
 * for code-generation and detect semantic errors. This includes invalid operand types,
 * invalid lvalues, and perform implicit conversions.
 * 
 * Expressions:
 * 
 *  - Annotate identifier primary nodes with symbol information
 * 
 *  - Check valid lvalues in assignments
 * 
 *  - Check operand types are appropriate, E.g.:
 *    - `a` is a function type in `a()`
 *    - `a` is a pointer in `a[2]` 
 *    - `a+n`, `a` and `n` are both arithmetic types
 * 
 * - Insert cast nodes in the AST to perform integer promotion, and arithmetic conversions
 * 
 *  - Insert cast nodes in the AST to convert:
 *    - Assignment expressions to the appropriate type
 *    - Initializer values to the appropriate type
 *    - Argument values in function calls to the corresponding parameter type
 * 
 * Declarations and Statements:
 * 
 *  - Allocate addresses (stack or global) for all object declarations.
 * 
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