#include <stdlib.h>
#include <string.h>

#include "ir.h"
#include "ir_gen.h"

#define EMIT(irgen, ...) emit(irgen, (IrInstruction){__VA_ARGS__})

typedef struct IrGenerator
{
    IrProgram *program;
    IrFunction *current_function;
    IrBasicBlock *current_basic_block;

    int current_function_local_index;
    int global_object_index;

    int register_counter;
} IrGenerator;

static IrRegister *walk_expr(IrGenerator *, ExprAstNode *);
void walk_stmt(IrGenerator *irgen, StmtAstNode *node);

/*
 * Helper functions
 */
static IrRegister *get_register(IrGenerator *irgen)
{
    IrRegister *reg = calloc(1, sizeof(IrRegister));
    reg->index = irgen->register_counter++;
    return reg;
}
static IrObject *allocate_local(IrGenerator *irgen, int size, int align, char *name)
{
    IrObject *new = calloc(1, sizeof(IrObject));
    new->name = name;
    new->size = 1;
    new->alignment = 1;
    new->index = irgen->current_function_local_index++;

    IrObject **ptr;
    for (ptr = &(irgen->current_function->locals); *ptr; ptr = &(*ptr)->next)
        ;
    *ptr = new;

    return new;
}
static IrObject *allocate_global(IrGenerator *irgen, int size, int align, char *name)
{
    IrObject *new = calloc(1, sizeof(IrObject));
    new->name = name;
    new->size = 1;
    new->alignment = 1;

    IrObject **ptr;
    for (ptr = &(irgen->program->globals); *ptr; ptr = &(*ptr)->next)
        ;
    *ptr = new;

    return new;
}
static IrBasicBlock *new_basic_block(IrGenerator *irgen)
{
    return calloc(1, sizeof(IrBasicBlock));
}
static void emit(IrGenerator *irgen, IrInstruction instr)
{
    IrInstruction *new = calloc(1, sizeof(IrInstruction));
    memcpy(new, &instr, sizeof(IrInstruction));

    IrInstruction **ptr;
    for (ptr = &(irgen->current_basic_block->head); *ptr; ptr = &(*ptr)->next)
        ;
    *ptr = new;
}

IrRegister *walk_expr_binary(IrGenerator *irgen, ExprAstNode *node)
{
    IrRegister *dest = get_register(irgen);
    IrRegister *left = walk_expr(irgen, node->binary.left);
    IrRegister *right = walk_expr(irgen, node->binary.right);

    switch(node->binary.op) {
        case BINARY_ADD:
            EMIT(irgen, ADD, .dest=dest, .left=left, .right=right);
            break;
        case BINARY_MUL:
            EMIT(irgen, MUL, .dest=dest, .left=left, .right=right);
            break;
        case BINARY_DIV:
            EMIT(irgen, DIV, .dest=dest, .left=left, .right=right);
            break;
        case BINARY_MOD:
            EMIT(irgen, MOD, .dest=dest, .left=left, .right=right);
            break;
        case BINARY_SUB:
            EMIT(irgen, SUB, .dest=dest, .left=left, .right=right);
            break;
        case BINARY_SLL:
            EMIT(irgen, SLL, .dest=dest, .left=left, .right=right);
            break;
        case BINARY_SLR:
            EMIT(irgen, SLR, .dest=dest, .left=left, .right=right);
            break;
        case BINARY_OR:
            EMIT(irgen, OR, .dest=dest, .left=left, .right=right);
            break;
        case BINARY_AND:
            EMIT(irgen, AND, .dest=dest, .left=left, .right=right);
            break;
        case BINARY_LT:
        case BINARY_GT:
        case BINARY_LE:
        case BINARY_GE:
        case BINARY_EQ:
        case BINARY_NE:
        case BINARY_XOR:
        case BINARY_AND_OP:
        case BINARY_OR_OP:
            break;
    }
    return dest;
}

IrRegister *walk_expr_unary(IrGenerator *irgen, ExprAstNode *node)
{
    switch(node->unary.op) {
        case UNARY_ADDRESS_OF:
            return walk_expr(irgen, node->unary.right);
        case UNARY_DEREFERENCE:
            {
                IrRegister * loc = walk_expr(irgen, node->unary.right);
                IrRegister * dest = get_register(irgen);
                dest->type = REGISTER_ADDRESS;
                EMIT(irgen, LOAD, .dest=dest, .left=loc);
                return dest;
            }
        case UNARY_PLUS:
        case UNARY_MINUS:
        case UNARY_BITWISE_NOT:
        case UNARY_LOGICAL_NOT:
        case UNARY_SIZEOF:
        case UNARY_INC_OP:
            {
                IrRegister * imm = get_register(irgen);
                IrRegister * operand = walk_expr(irgen, node->unary.right);
                EMIT(irgen, LOADI, .dest=imm, .immediate.type=IMMEDIATE_VALUE, .immediate.value=1);
                EMIT(irgen, ADD, .dest=operand, .left=imm, .right=operand);
                return operand;
            }
        case UNARY_DEC_OP:
            {
                IrRegister * imm = get_register(irgen);
                IrRegister * operand = walk_expr(irgen, node->unary.right);
                EMIT(irgen, LOADI, .dest=imm, .immediate.type=IMMEDIATE_VALUE, .immediate.value=1);
                EMIT(irgen, SUB, .dest=operand, .left=imm, .right=operand);
                return operand;
            }
            break;
    }
    return NULL;
}

IrRegister *walk_expr_primary(IrGenerator *irgen, ExprAstNode *node)
{
    if (node->primary.constant)
    {
        IrRegister *reg = get_register(irgen);
        EMIT(irgen, LOADI, .dest = reg, .immediate.type = IMMEDIATE_VALUE,
             .immediate.value = node->primary.constant->literal.const_value);
        return reg;
    }
    else if (node->primary.identifier)
    {
        Symbol *symbol = node->primary.symbol;
        if (symbol->ir.regster)
        {
            return symbol->ir.regster;
        }
        else
        {
            IrRegister *reg = get_register(irgen);
            EMIT(irgen, LOADI, .dest = reg, .immediate.type = IMMEDIATE_OBJECT,
                 .immediate.object = symbol->ir.object);
            return reg;
        }
    }
    return NULL;
}

IrRegister *walk_expr_postfix(IrGenerator *irgen, ExprAstNode *node)
{    
    switch(node->postfix.op) {
        case POSTFIX_CALL:
        case POSTFIX_INC_OP:
        {
            IrRegister * operand = walk_expr(irgen, node->postfix.left);
            IrRegister * imm = get_register(irgen), * dest = get_register(irgen);
            EMIT(irgen, LOADI, .dest=imm, .immediate.type=IMMEDIATE_VALUE, .immediate.value=1);
            EMIT(irgen, MOV, .dest=dest, .left=operand);
            EMIT(irgen, ADD, .dest=dest, .left=dest, .right=imm);
            return operand;
        }
        case POSTFIX_DEC_OP:
        {
            IrRegister * operand = walk_expr(irgen, node->postfix.left);
            IrRegister * imm = get_register(irgen), * dest = get_register(irgen);
            EMIT(irgen, LOADI, .dest=imm, .immediate.type=IMMEDIATE_VALUE, .immediate.value=1);
            EMIT(irgen, MOV, .dest=dest, .left=operand);
            EMIT(irgen, SUB, .dest=dest, .left=dest, .right=imm);
            return operand;
        }
    }
    return NULL;
}

IrRegister *walk_expr_cast(IrGenerator *irgen, ExprAstNode *node)
{
    return NULL;
}

IrRegister *walk_expr_tertiary(IrGenerator *irgen, ExprAstNode *node)
{
    return NULL;
}
IrRegister *walk_expr_assign(IrGenerator *irgen, ExprAstNode *node)
{
    IrRegister *dest = walk_expr(irgen, node->assign.left);

    if (dest->type == REGISTER_VALUE)
    {
        // `dest` is the actual register for this object.
        IrRegister *src = walk_expr(irgen, node->assign.right);
        EMIT(irgen, MOV, .dest = dest, .left = src);
    }
    else
    {
        // `dest` holds the address for this object in memory.
        IrRegister *src = walk_expr(irgen, node->assign.right);
        EMIT(irgen, STORE, .left = dest, .dest = src);
    }
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
    IrFunction *func = calloc(1, sizeof(IrFunction));

    func->name = node->identifier->lexeme;
    func->entry = new_basic_block(irgen);

    func->next = irgen->program->functions;
    irgen->program->functions = func;

    irgen->current_function = func;
    irgen->current_basic_block = func->entry;

    irgen->register_counter = 0;

    // Create new registers for the parameters.
    for(ExprAstNode * param = node->type->derived.params->)

    walk_stmt(irgen, node->body);

    irgen->current_function = NULL;
}

void walk_decl_object(IrGenerator *irgen, DeclAstNode *node)
{
    if (!irgen->current_function)
    {
        node->symbol->ir.object = allocate_global(irgen, 1, 1, node->identifier->lexeme);
    }
    else
    {
        if (CTYPE_IS_SCALAR(node->type))
        {
            IrRegister *reg = get_register(irgen);
            reg->type = REGISTER_VALUE;
            node->symbol->ir.regster = reg;

            if(node->initializer) {
                IrRegister * init_reg = walk_expr(irgen, node->initializer);
                EMIT(irgen, MOV, .dest=reg, .left=init_reg);
            }
        }
        else
        {
            node->symbol->ir.object =
                allocate_local(irgen, 1, 1, node->identifier->lexeme);
        }
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
}

void walk_stmt_return(IrGenerator *irgen, StmtAstNode *node)
{
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