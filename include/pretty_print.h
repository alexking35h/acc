#ifndef __PRETTY_PRINT__
#define __PRETTY_PRINT__

#include <stdio.h>
#include <string.h>

#include "ast.h"
#include "pretty_print.h"

/*
 * Generate a string for the given ExprAstNode.
 */
int pretty_print_expr(ExprAstNode* node, char* buf, int len);

/*
 * Generate a string for the given DeclAstNode.
 */
int pretty_print_decl(DeclAstNode* node, char* buf, int len);

#endif