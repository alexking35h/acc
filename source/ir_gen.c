#include <stdlib.h>
#include <string.h>

#include "arch.h"
#include "ir.h"
#include "ir_gen.h"

#define MAX(a,b) (a < b ? b : a)

#define EMIT_INSTR(irgen, opcode, ...) \
    emit_instr(irgen->current_basic_block, (IrInstruction){.op=opcode, __VA_ARGS__})

typedef struct IrGenerator
{
    IrProgram *program;

    IrFunction *current_function;
    IrBasicBlock *current_function_exit_bb;

    IrBasicBlock *current_basic_block;

    IrRegister *argument_registers[4];

    int bb_counter;
} IrGenerator;

static IrRegister *walk_expr(IrGenerator *, ExprAstNode *);
static void walk_stmt(IrGenerator *irgen, StmtAstNode *node);

static void emit_instr(IrBasicBlock * bb, IrInstruction instr) {
    IrInstruction * new_instr = calloc(1, sizeof(IrInstruction));

    memcpy(new_instr, &instr, sizeof(IrInstruction));

    if(bb->head == NULL) {
        bb->head = bb->tail = new_instr;
    } else {
        bb->tail->next = new_instr;
        bb->tail = new_instr;
    }
    new_instr->next = NULL;
}

static IrBasicBlock * new_bb(IrGenerator * irgen, IrFunction * function) {
    IrBasicBlock * bb = calloc(1, sizeof(IrBasicBlock));
    bb->index = irgen->bb_counter++;
    emit_instr(bb, (IrInstruction){IR_NOP});

    if(function->head == NULL)
    {
        function->head = function->tail = bb;
    } else {
        function->tail->next = bb;
        function->tail = bb;
    }
    return bb;
}

static IrFunction * new_function(IrGenerator * irgen, char * name)
{
    IrFunction * function = calloc(1, sizeof(IrFunction));
    function->next = irgen->program->functions;
    function->name = name;

    irgen->program->functions = function;
    return function;
}

static IrRegister *new_reg(IrGenerator *irgen, IrRegType type)
{
    IrRegister * reg = calloc(1, sizeof(IrRegister));
    switch(reg->type = type) {
        case REG_ANY:
            reg->index = irgen->program->register_count.any++;
            break;
        case REG_ARGUMENT:
            reg->index = irgen->program->register_count.arg++;
            break;
        case REG_RETURN:
            reg->index = 0;
            break;
    }
    return reg;
}

static IrRegister * new_arg_reg(IrGenerator *irgen, bool beginning)
{
    static int reg_index;
    if(beginning)
        reg_index = 0;

    return irgen->argument_registers[reg_index++];
}

static IrObject * stack_allocate(IrFunction * function, Symbol * symbol)
{
    // Allocate space for a symbol on the stack.
    int size = arch_get_size(symbol->type);
    int align = arch_get_align(symbol->type);
    int sign = arch_get_signed(symbol->type);
    int offset = function->stack_size;

    // For now, just give everything on the stack 4 bytes.
    function->stack_size += MAX((size + 3) & ~3, 4);

    IrObject * object = calloc(1, sizeof(IrObject));
    object->size = size;
    object->align = align;
    object->sign = sign;
    object->offset = offset;
    object->storage = LOCAL;

    return object;
}

static IrObject * bss_allocate(IrProgram * program, Symbol * symbol)
{
    // Allocate space for a symbol in BSS.
    abort();
}

static IrRegister *walk_expr_binary(IrGenerator *irgen, ExprAstNode *node)
{
    IrOpcode op;
    bool post_op_negate = false;
    switch (node->binary.op)
    {
    case BINARY_ADD:
        op = IR_ADD;
        break;
    case BINARY_SUB:
        op = IR_SUB;
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
        op = IR_LT;
        break;
    case BINARY_GT:
        op = IR_LE;
        post_op_negate = true;
        break;
    case BINARY_LE:
        abort();
        break;
    case BINARY_GE:
        abort();
        break;
    case BINARY_EQ:
        op = IR_EQ;
        break;
    case BINARY_NE:
        op = IR_EQ;
        post_op_negate = true;
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
    IrRegister * dest = new_reg(irgen, REG_ANY);
    IrRegister * left = walk_expr(irgen, node->binary.left);
    IrRegister * right = walk_expr(irgen, node->binary.right);
    EMIT_INSTR(irgen, op, .dest=dest, .left=left, .right=right);

    if(post_op_negate)
    {
        EMIT_INSTR(irgen, IR_NOT, .dest=dest, .left=dest);
    }
    return dest;
}

static IrRegister *walk_expr_unary(IrGenerator *irgen, ExprAstNode *node)
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

static IrRegister *walk_expr_primary(IrGenerator *irgen, ExprAstNode *node)
{
    if (node->primary.constant)
    {
        IrRegister * reg = new_reg(irgen, REG_ANY);
        int val = node->primary.constant->literal.const_value;
        EMIT_INSTR(irgen, IR_LOADI, .dest=reg, .value=val);
        return reg;
    }
    else if (node->primary.identifier)
    {
        Symbol * sym = node->primary.symbol;
        if(sym->ir.object) {
            // Object is not in a register.
            abort();
        } else if(sym->ir.regster)
        {
            // Object is in a register.
            return sym->ir.regster;
        }
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
}

static IrRegister *walk_expr_postfix_call(IrGenerator *irgen, ExprAstNode *node)
{
    // What to do here?
    if(node->postfix.left->type == UNARY && node->postfix.left->unary.op == UNARY_DEREFERENCE)
    {
        // This is a function pointer. Urgh, what a pain.
        // Think on this one alex.
        abort();
    } else {
        // This is just a straight up function call :-)
        IrFunction * func = node->postfix.left->primary.symbol->ir.function;
        int beginning = 0;
        for(ArgumentListItem * arg = node->postfix.args;arg != NULL;arg = arg->next)
        {
            IrRegister * arg_reg = walk_expr(irgen, arg->argument);
            IrRegister * param_reg = new_arg_reg(irgen, beginning++ == 0);
            EMIT_INSTR(irgen, IR_MOV, .dest=param_reg, .left=arg_reg);
        }
        EMIT_INSTR(irgen, IR_CALL, .function=func);
        return new_reg(irgen, REG_RETURN);
    }
}

static IrRegister *walk_expr_postfix(IrGenerator *irgen, ExprAstNode *node)
{
    switch (node->postfix.op)
    {
    case POSTFIX_CALL:
        return walk_expr_postfix_call(irgen, node);

    case POSTFIX_INC_OP:
        abort();
        break;
    case POSTFIX_DEC_OP:
        abort();
        break;
    }
    return NULL;
}

static IrRegister *walk_expr_cast(IrGenerator *irgen, ExprAstNode *node)
{
    abort();
    return NULL;
}

static IrRegister *walk_expr_tertiary(IrGenerator *irgen, ExprAstNode *node)
{
    abort();
    return NULL;
}
static IrRegister *walk_expr_assign(IrGenerator *irgen, ExprAstNode *node)
{
    if(node->assign.left->type == UNARY && node->assign.left->unary.op == UNARY_DEREFERENCE)
    {
        abort();
    } else {
        IrRegister * dest = walk_expr(irgen, node->assign.left);
        IrRegister * value = walk_expr(irgen, node->assign.right);
        EMIT_INSTR(irgen, IR_MOV, .dest=dest, .left=value);
        return dest;
    }
}

static IrRegister *walk_expr(IrGenerator *irgen, ExprAstNode *node)
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

static void walk_decl_function(IrGenerator *irgen, DeclAstNode *node)
{
    IrFunction * func = new_function(irgen, node->identifier->lexeme);
    node->symbol->ir.function = func;
    irgen->current_function = func;

    irgen->current_basic_block = new_bb(irgen, func);
    EMIT_INSTR(irgen, IR_STACK);

    int i = 0;
    for(ActualParameterListItem *arg = node->args;arg != NULL;arg = arg->next)
    {
        IrRegister * argument_body = new_reg(irgen, REG_ANY);
        IrRegister * argument_proto = new_arg_reg(irgen, i++ == 0);

        arg->sym->ir.regster = argument_body;
        EMIT_INSTR(irgen, IR_MOV, .dest=argument_body, .left=argument_proto);
    }
    
    irgen->current_function_exit_bb = new_bb(irgen, func);

    walk_stmt(irgen, node->body);
    EMIT_INSTR(irgen, IR_JUMP, .jump=irgen->current_function_exit_bb);

    irgen->current_basic_block = irgen->current_function_exit_bb;
    EMIT_INSTR(irgen, IR_UNSTACK);
    EMIT_INSTR(irgen, IR_RETURN);
}

static void walk_decl_object(IrGenerator *irgen, DeclAstNode *node)
{
    if (!irgen->current_function)
    {
        // Global
        abort();
    }
    else if(CTYPE_IS_SCALAR(node->type))
    {
        // Local - allocate in register.
        IrRegister * reg = new_reg(irgen, REG_ANY);
        node->symbol->ir.regster = reg;

        if(node->initializer)
        {
            IrRegister * init = walk_expr(irgen, node->initializer);
            EMIT_INSTR(irgen, IR_MOV, .dest=reg, .left=init);
        }
    } else {
        // Local - allocate on the stack.
        IrObject * object = stack_allocate(
            irgen->current_function,
            node->symbol
        );
        node->symbol->ir.object = object;

        if(node->initializer)
        {
            abort();
        }
    }
}

static void walk_decl(IrGenerator *irgen, DeclAstNode *node)
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

static void walk_stmt_while(IrGenerator *irgen, StmtAstNode *node)
{
    IrBasicBlock * loop_init_bb = new_bb(irgen, irgen->current_function);
    IrBasicBlock * while_bb = new_bb(irgen, irgen->current_function);
    IrBasicBlock * end_bb = new_bb(irgen, irgen->current_function);

    EMIT_INSTR(irgen, IR_JUMP, .jump=loop_init_bb);
    irgen->current_basic_block = loop_init_bb;

    IrRegister * condition_reg = walk_expr(irgen, node->while_loop.expr);
    EMIT_INSTR(irgen, IR_BRANCHZ, .left=condition_reg, .jump_false=end_bb, .jump_true=while_bb);

    irgen->current_basic_block = while_bb;
    walk_stmt(irgen, node->while_loop.block);
    EMIT_INSTR(irgen, IR_JUMP, .jump=loop_init_bb);

    irgen->current_basic_block = end_bb;
}

static void walk_stmt_return(IrGenerator *irgen, StmtAstNode *node)
{
    if(node->return_jump.value) {
        IrRegister * reg = new_reg(irgen, REG_RETURN);
        IrRegister * val = walk_expr(irgen, node->return_jump.value);
        EMIT_INSTR(irgen, IR_MOV, .dest=reg, .left=val);

        // Note: need to issue a 'return' here.
        EMIT_INSTR(irgen, IR_JUMP, .jump=irgen->current_function_exit_bb);
    }
}

static void walk_stmt(IrGenerator *irgen, StmtAstNode *node)
{
    if(node == NULL) return;

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

    walk_stmt(irgen, node->next);
}

IrProgram *Ir_generate(DeclAstNode *ast_root)
{
    IrGenerator irgen = {
        .program = calloc(1, sizeof(IrProgram)),
    };
    for(int i = 0;i < 4;i++)
    {
        irgen.argument_registers[i] = new_reg(&irgen, REG_ARGUMENT);
    }

    walk_decl(&irgen, ast_root);
    return irgen.program;
}