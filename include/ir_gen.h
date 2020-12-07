
#include "ast.h"
#include "ir.h"
#include "symbol.h"

/*
 * Generate the IR representation for the given program
 */
IrFunction *Ir_generate(DeclAstNode *, SymbolTable*);