#include <stdlib.h>
#include <string.h>

#include "ir.h"
#include "ir_gen.h"

typedef struct IrGenerator
{
    IrProgram *program;
    IrFunction *current_function;
    IrBasicBlock *current_basic_block;

    int current_function_local_index;
    int global_object_index;

    int register_counter;
} IrGenerator;

static IrRegister *allocate_register(IrGenerator *irgen, IrRegType type)
{
    abort();
}
static IrObject *allocate_local(IrGenerator *irgen)
{
    abort();
}
static IrObject *allocate_global(IrGenerator *irgen)
{
    abort();
}

static IrRegister *walk_expr(IrGenerator *, ExprAstNode *);
void walk_stmt(IrGenerator *irgen, StmtAstNode *node);

IrRegister *walk_expr_binary(IrGenerator *irgen, ExprAstNode *node)
{
    switch (node->binary.op)
    {
    case BINARY_ADD:
        abort();
        break;
    case BINARY_SUB:
        abort();
        break;
    case BINARY_MUL:
        abort();
        break;
    case BINARY_DIV:
        abort();
        break;
    case BINARY_MOD:
        abort();
        break;
    case BINARY_SLL:
        abort();
        break;
    case BINARY_SLR:
        abort();
        break;
    case BINARY_LT:
        abort();
        break;
    case BINARY_GT:
        abort();
        break;
    case BINARY_LE:
        abort();
        break;
    case BINARY_GE:
        abort();
        break;
    case BINARY_EQ:
        abort();
        break;
    case BINARY_NE:
        abort();
        break;
    case BINARY_AND:
        abort();
        break;
    case BINARY_OR:
        abort();
        break;
    case BINARY_XOR:
        abort();
        break;
    case BINARY_AND_OP:
        abort();
        break;
    case BINARY_OR_OP:
        abort();
        break;
    }
    return NULL;
}

IrRegister *walk_expr_unary(IrGenerator *irgen, ExprAstNode *node)
{
    switch (node->unary.op)
    {
    case UNARY_ADDRESS_OF:
        abort();
        break;
    case UNARY_DEREFERENCE:
        abort();
        break;
    case UNARY_PLUS:
        abort();
        break;
    case UNARY_MINUS:
        abort();
        break;
    case UNARY_BITWISE_NOT:
        abort();
        break;
    case UNARY_LOGICAL_NOT:
        abort();
        break;
    case UNARY_SIZEOF:
        abort();
        break;
    case UNARY_INC_OP:
        abort();
        break;
    case UNARY_DEC_OP:
        abort();
        break;
    }
    return NULL;
}

IrRegister *walk_expr_primary(IrGenerator *irgen, ExprAstNode *node)
{
    if (node->primary.constant)
    {
        abort();
    }
    else if (node->primary.identifier)
    {
        abort();
    }
    else if (node->primary.string_literal)
    {
        abort();
    }
    else if (node->primary.symbol)
    {
        abort();
    }
    else
    {
        abort();
    }
    return NULL;
}

IrRegister *walk_expr_postfix(IrGenerator *irgen, ExprAstNode *node)
{
    switch (node->postfix.op)
    {
    case POSTFIX_CALL:
        abort();
        break;
    case POSTFIX_INC_OP:
        abort();
        break;
    case POSTFIX_DEC_OP:
        abort();
        break;
    }
    return NULL;
}

IrRegister *walk_expr_cast(IrGenerator *irgen, ExprAstNode *node)
{
    abort();
    return NULL;
}

IrRegister *walk_expr_tertiary(IrGenerator *irgen, ExprAstNode *node)
{
    abort();
    return NULL;
}
IrRegister *walk_expr_assign(IrGenerator *irgen, ExprAstNode *node)
{
    abort();
    return NULL;
}

IrRegister *walk_expr(IrGenerator *irgen, ExprAstNode *node)
{
    switch (node->type)
    {
    case BINARY:
        return walk_expr_binary(irgen, node);
    case UNARY:
        return walk_expr_unary(irgen, node);
    case PRIMARY:
        return walk_expr_primary(irgen, node);
    case POSTFIX:
        return walk_expr_postfix(irgen, node);
    case CAST:
        return walk_expr_cast(irgen, node);
    case TERTIARY:
        return walk_expr_tertiary(irgen, node);
    case ASSIGN:
        return walk_expr_assign(irgen, node);
    }
    return NULL;
}

void walk_decl_function(IrGenerator *irgen, DeclAstNode *node)
{
    abort();
}

void walk_decl_object(IrGenerator *irgen, DeclAstNode *node)
{
    if (!irgen->current_function)
    {
        // Global
        abort();
    }
    else
    {
        // Local
        abort();
    }
}

void walk_decl(IrGenerator *irgen, DeclAstNode *node)
{
    if (node->body)
    {
        // We are declaring a new function
        walk_decl_function(irgen, node);
    }
    else
    {
        // We are declaring an object.
        walk_decl_object(irgen, node);
    }

    if (node->next)
    {
        walk_decl(irgen, node->next);
    }
}

void walk_stmt_while(IrGenerator *irgen, StmtAstNode *node)
{
    abort();
}

void walk_stmt_return(IrGenerator *irgen, StmtAstNode *node)
{
    abort();
}

void walk_stmt(IrGenerator *irgen, StmtAstNode *node)
{
    switch (node->type)
    {
    case DECL:
        walk_decl(irgen, node->decl.decl);
        break;
    case EXPR:
        walk_expr(irgen, node->expr.expr);
        break;
    case BLOCK:
        walk_stmt(irgen, node->block.head);
        break;
    case WHILE_LOOP:
        walk_stmt_while(irgen, node);
        break;
    case RETURN_JUMP:
        walk_stmt_return(irgen, node);
        break;
    }

    if (node->next)
    {
        walk_stmt(irgen, node->next);
    }
}

IrProgram *Ir_generate(DeclAstNode *ast_root)
{
    IrGenerator irgen = {.program = calloc(1, sizeof(IrProgram))};

    walk_decl(&irgen, ast_root);
    return irgen.program;
}