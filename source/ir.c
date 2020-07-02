#include <stdio.h>
#include <stdlib.h>

#include "ir.h"

typedef struct CharBuffer
{
    char *pos;
    char *end;
} CharBuffer;

#define SNPRINTF(cb, ...)                                                           \
    cb->pos += snprintf(cb->pos, cb->end - cb->pos, __VA_ARGS__)

static char *opcode_str(IrOpcode op)
{
    switch (op)
    {
    case ADD:
        return "add";
    case SUB:
        return "sub";
    case MUL:
        return "mul";
    case DIV:
        return "div";
    case MOD:
        return "mod";
    case SLL:
        return "sll";
    case SLR:
        return "slr";
    case OR:
        return "or";
    case AND:
        return "and";
    case MOV:
        return "mov";
    case STORE:
        return "store";
    case LOAD:
        return "load";
    case LOADI:
        return "loadi";
    case TYPE_CAST:
        return "cast";
    case COMPARE:
        return "cmp";
    case BRANCH:
        return "branch";
    case JUMP:
        return "jmp";
    case CALL:
        return "call";
    case RET:
        return "ret";
    }
}

static void instruction_arithmetic(CharBuffer *buf, IrInstruction *instr)
{
    switch (instr->op)
    {
    case ADD:
    case SUB:
    case MUL:
    case DIV:
    case MOD:
    case SLL:
    case SLR:
    case OR:
    case AND:
        SNPRINTF(buf, "  %s r%d, r%d, r%d\n", opcode_str(instr->op), instr->dest->index,
                 instr->left->index, instr->right->index);
        break;

    case MOV:
    case STORE:
    case LOAD:
        SNPRINTF(buf, "  %s r%d, r%d\n", opcode_str(instr->op), instr->dest->index,
                 instr->left->index);
        break;
    }
}

static void instruction_immediate(CharBuffer *buf, IrInstruction *instr)
{
    switch (instr->immediate.type)
    {
    case IMMEDIATE_VALUE:
        SNPRINTF(buf, "  %s r%d, !%d\n", opcode_str(instr->op), instr->dest->index,
                 instr->immediate.value);
        break;
    case IMMEDIATE_OBJECT:
        SNPRINTF(buf, "  %s r%d, @%s\n", opcode_str(instr->op), instr->dest->index,
                 instr->immediate.object->name);
        break;
    }
}

static void instruction_cast(CharBuffer *buf, IrInstruction *instr)
{
    // @TODO
}

static void instruction_jump(CharBuffer *buf, IrInstruction *instr)
{
    SNPRINTF(buf, "  %s %s", opcode_str(instr->op), instr->jump_true->label);
    if (instr->op == BRANCH)
    {
        SNPRINTF(buf, ", %s\n", instr->jump_false->label);
    }
    else 
    {
        SNPRINTF(buf, "\n");
    }
}

static void instruction_call(CharBuffer *buf, IrInstruction *instr)
{
    // @TODO
}

static void instruction_return(CharBuffer *buf, IrInstruction *instr)
{
    // @TODO
}

static void instruction(CharBuffer *buf, IrInstruction *instr)
{
    switch (instr->op)
    {
    case ADD:
    case SUB:
    case MUL:
    case DIV:
    case MOD:
    case SLL:
    case SLR:
    case OR:
    case AND:
    case COMPARE:
    case STORE:
    case LOAD:
        instruction_arithmetic(buf, instr);
        break;

    case LOADI:
        instruction_immediate(buf, instr);
        break;

    case TYPE_CAST:
        instruction_cast(buf, instr);
        break;

    case BRANCH:
    case JUMP:
        instruction_jump(buf, instr);

    case CALL:
        instruction_call(buf, instr);
        break;

    case RET:
        instruction_return(buf, instr);
        break;
    }
}

static void basic_block_graph(CharBuffer *buf, IrBasicBlock *block)
{
    IrBasicBlock *todo[32] = {block};
    int head = 0, tail = 31;

    while ((head % 32) != (tail % 32))
    {
        IrBasicBlock *bb = todo[head];
        for (IrInstruction *instr = bb->head; instr; instr = instr->next)
        {
            instruction(buf, instr);

            if (instr->jump_false)
            {
                todo[(head++) % 32] = instr->jump_false;
            }
            if (instr->jump_true)
            {
                todo[(head++) % 32] = instr->jump_true;
            }
        }
        tail++;
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

    // Print out all blocks.
    basic_block_graph(buf, func->entry);
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