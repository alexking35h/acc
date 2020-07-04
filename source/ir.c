#include <stdio.h>
#include <stdlib.h>

#include "ir.h"

typedef struct CharBuffer
{
    char *pos;
    char *end;
} CharBuffer;

#define SNPRINTF(cb, ...) cb->pos += snprintf(cb->pos, cb->end - cb->pos, __VA_ARGS__)

static char *opcode_str(IrOpcode op)
{
    switch (op)
    {
    case IR_ADD:
        return "add";
    case IR_SUB:
        return "sub";
    case IR_MUL:
        return "mul";
    case IR_DIV:
        return "div";
    case IR_MOD:
        return "mod";
    case IR_SLL:
        return "sll";
    case IR_SLR:
        return "slr";
    case IR_OR:
        return "or";
    case IR_AND:
        return "and";
    case IR_NOT:
        return "not";
    case IR_EQ:
        return "eq";
    case IR_LT:
        return "lt";
    case IR_LE:
        return "le";
    case IR_MOV:
        return "mov";
    case IR_STORE:
        return "store";
    case IR_LOAD:
        return "load";
    case IR_LOADI:
        return "loadi";
    case IR_LOADA:
        return "loada";
    case IR_BRANCHZ:
        return "branchz";
    case IR_JUMP:
        return "jmp";
    case IR_CALL:
        return "call";
    case IR_RETURN:
        return "ret";
    }
    return NULL;
}

static void reg(CharBuffer *buf, IrRegister *reg)
{
    switch (reg->type)
    {
    case REG_ARGUMENT:
        SNPRINTF(buf, "a%d", reg->index);
        break;
    case REG_RETURN:
        SNPRINTF(buf, "r%d", reg->index);
        break;
    case REG_ANY:
        SNPRINTF(buf, "t%d", reg->index);
    }
}

static void instruction_arithmetic(CharBuffer *buf, IrInstruction *instr)
{
    SNPRINTF(buf, "    %s ", opcode_str(instr->op));
    reg(buf, instr->dest);
    SNPRINTF(buf, ", ");
    reg(buf, instr->left);
    SNPRINTF(buf, ", ");
    reg(buf, instr->right);
    SNPRINTF(buf, "\n");
}

static void instruction_move(CharBuffer *buf, IrInstruction *instr)
{
    SNPRINTF(buf, "    %s ", opcode_str(instr->op));
    reg(buf, instr->dest);
    SNPRINTF(buf, ", ");
    reg(buf, instr->left);
    SNPRINTF(buf, "\n");
}

static void instruction_load(CharBuffer *buf, IrInstruction *instr)
{
    SNPRINTF(buf, "    %s ", opcode_str(instr->op));
    reg(buf, instr->dest);
    if (instr->op == IR_LOADI)
    {
        SNPRINTF(buf, ", !%d\n", instr->value);
    }
    else
    {
        SNPRINTF(buf, ", @%s\n", instr->object->name);
    }
}

static void instruction_mem(CharBuffer *buf, IrInstruction *instr)
{
    SNPRINTF(buf, "    %s ", opcode_str(instr->op));
    if (instr->op == IR_LOAD)
    {
        reg(buf, instr->dest);
        SNPRINTF(buf, ", ");
        reg(buf, instr->left);
    }
    else
    {
        reg(buf, instr->left);
        SNPRINTF(buf, ", ");
        reg(buf, instr->right);
    }
    SNPRINTF(buf, "\n");
}

static void instruction_jump(CharBuffer *buf, IrInstruction *instr)
{
    SNPRINTF(buf, "    %s %s\n", opcode_str(instr->op), instr->jump->label);
}

static void instruction(CharBuffer *buf, IrInstruction *instr)
{
    switch (instr->op)
    {
    case IR_ADD:
    case IR_SUB:
    case IR_MUL:
    case IR_DIV:
    case IR_MOD:
    case IR_SLL:
    case IR_SLR:
    case IR_OR:
    case IR_AND:
    case IR_NOT:
    case IR_EQ:
    case IR_LT:
    case IR_LE:
        instruction_arithmetic(buf, instr);
        break;

    case IR_MOV:
        instruction_move(buf, instr);
        break;

    case IR_LOADI:
    case IR_LOADA:
        instruction_load(buf, instr);
        break;

    case IR_LOAD:
    case IR_STORE:
        instruction_mem(buf, instr);

    case IR_BRANCHZ:
    case IR_JUMP:
    case IR_CALL:
    case IR_RETURN:
        instruction_jump(buf, instr);
    }
}

static void basic_block(CharBuffer *buf, IrBasicBlock *block)
{
    SNPRINTF(buf, "  .bb %s\n", block->label);

    for (IrInstruction *instr = block->head; instr != NULL; instr = instr->next)
    {
        instruction(buf, instr);
    }
}

static void object(CharBuffer *buf, IrObject *object, char *label)
{
    SNPRINTF(buf, "%s %s=%d,%d\n", label, object->name, object->size, object->alignment);
}

static void function(CharBuffer *buf, IrFunction *func)
{
    SNPRINTF(buf, "\n.fun %s:\n", func->name);

    // Print out all local variables.
    for (IrObject *obj = func->locals; obj != NULL; obj = obj->next)
    {
        object(buf, obj, "  .local");
    }

    for (IrBasicBlock *block = func->entry; block != NULL; block = block->next)
    {
        basic_block(buf, block);
    }
}

static void program(CharBuffer *buf, IrProgram *prog)
{
    // Print out all global variables.
    for (IrObject *obj = prog->globals; obj != NULL; obj = obj->next)
    {
        object(buf, obj, ".global");
    }

    // Print out all functions.
    for (IrFunction *fun = prog->functions; fun != NULL; fun = fun->next)
    {
        function(buf, fun);
    }
}

char *Ir_to_str(IrProgram *ir)
{
    char *chbuffer = calloc(4096, sizeof(char));
    CharBuffer buffer = {chbuffer, chbuffer + 4096};

    program(&buffer, ir);

    return chbuffer;
}