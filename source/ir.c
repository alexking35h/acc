#include <stdio.h>
#include <stdlib.h>

#include "ir.h"

#define INDENT "    "

#define HEADER                                                                           \
    "// === ACC IR === \n//\n"                                                           \
    "// Date: " __DATE__ "\n"                                                            \
    "// Time: " __TIME__ "\n\n"                                                          \
    "#include <stdint.h>\n\n"

typedef struct CharBuffer
{
    char *pos;
    char *end;
} CharBuffer;

#define SNPRINTF(cb, ...) cb->pos += snprintf(cb->pos, cb->end - cb->pos, __VA_ARGS__)

static void ir_register(CharBuffer *buf, IrRegister *reg)
{
    switch (reg->type)
    {
    case REG_ANY:
        SNPRINTF(buf, "t%d", reg->index);
        break;
    case REG_ARGUMENT:
        SNPRINTF(buf, "a%d", reg->index);
        break;
    case REG_RETURN:
        SNPRINTF(buf, "r%d", reg->index);
        break;
    }
}

static void instruction_arithmetic(CharBuffer *buf, IrInstruction *instr)
{
    SNPRINTF(buf, INDENT);
    ir_register(buf, instr->dest);
    SNPRINTF(buf, " = ");

    if (instr->op == IR_NOT)
    {
        SNPRINTF(buf, "! ");
        ir_register(buf, instr->left);
        SNPRINTF(buf, ";\n");
        return;
    }

    ir_register(buf, instr->left);
    switch (instr->op)
    {
    case IR_ADD:
        SNPRINTF(buf, " + ");
        break;
    case IR_SUB:
        SNPRINTF(buf, " - ");
        break;
    case IR_MUL:
        SNPRINTF(buf, " * ");
        break;
    case IR_DIV:
        SNPRINTF(buf, " / ");
        break;
    case IR_MOD:
        SNPRINTF(buf, " %% ");
        break;
    case IR_SLL:
        SNPRINTF(buf, " << ");
        break;
    case IR_SLR:
        SNPRINTF(buf, " >> ");
        break;
    case IR_OR:
        SNPRINTF(buf, " | ");
        break;
    case IR_AND:
        SNPRINTF(buf, " & ");
        break;
    case IR_NOT:
        SNPRINTF(buf, " ! ");
        break;
    case IR_EQ:
        SNPRINTF(buf, " == ");
        break;
    case IR_LT:
        SNPRINTF(buf, " < ");
        break;
    case IR_LE:
        SNPRINTF(buf, " <= ");
        break;
    }
    ir_register(buf, instr->right);
    SNPRINTF(buf, ";\n");
}

static void instruction_move(CharBuffer *buf, IrInstruction *instr)
{
    SNPRINTF(buf, INDENT);
    ir_register(buf, instr->dest);
    SNPRINTF(buf, " = ");
    ir_register(buf, instr->left);
    SNPRINTF(buf, ";\n");
}

static void instruction_mem(CharBuffer *buf, IrInstruction *instr)
{
    SNPRINTF(buf, INDENT);
    if (instr->op == IR_LOAD)
    {
        ir_register(buf, instr->dest);
        SNPRINTF(buf, " = *");
        ir_register(buf, instr->left);
    }
    else
    {
        SNPRINTF(buf, "*");
        ir_register(buf, instr->left);
        SNPRINTF(buf, " = ");
        ir_register(buf, instr->right);
    }
    SNPRINTF(buf, ";\n");
}

static void instruction_loadi(CharBuffer *buf, IrInstruction *instr)
{
    SNPRINTF(buf, INDENT);
    ir_register(buf, instr->dest);
    SNPRINTF(buf, " = %d;\n", instr->value);
}

static void instruction_stack(CharBuffer *buf, IrInstruction *instr)
{
    if (instr->op == IR_STACK)
    {
        SNPRINTF(buf, INDENT "// STACK REGISTERS\n");
    }
    else
    {
        SNPRINTF(buf, INDENT "// UNSTACK REGISTERS\n");
    }
}

static void instruction_jump(CharBuffer *buf, IrInstruction *instr)
{
    if (instr->op == IR_JUMP)
    {
        SNPRINTF(buf, INDENT "goto bb_%d;\n", instr->jump->index);
    }
    else if (instr->op == IR_RETURN)
    {
        SNPRINTF(buf, INDENT "return;\n");
    }
    else if (instr->op == IR_BRANCHZ)
    {
        SNPRINTF(buf, INDENT "if(");
        ir_register(buf, instr->left);
        SNPRINTF(buf, ")\n" INDENT "{\n");
        SNPRINTF(buf, INDENT INDENT "goto bb_%d;\n", instr->jump_true->index);
        SNPRINTF(buf, INDENT "} else {\n");
        SNPRINTF(buf, INDENT INDENT "goto bb_%d;\n", instr->jump_false->index);
        SNPRINTF(buf, INDENT "}\n");
    }
    else if (instr->op == IR_CALL)
    {
        SNPRINTF(buf, INDENT "_%s();\n", instr->function->name);
    }
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

    case IR_STORE:
    case IR_LOAD:
        instruction_mem(buf, instr);
        break;

    case IR_LOADI:
        instruction_loadi(buf, instr);
        break;

    case IR_STACK:
    case IR_UNSTACK:
        instruction_stack(buf, instr);
        break;

    case IR_BRANCHZ:
    case IR_JUMP:
    case IR_CALL:
    case IR_RETURN:
        instruction_jump(buf, instr);
        break;
    case IR_NOP:
        SNPRINTF(buf, INDENT ";\n");
    }
}

static void basic_block(CharBuffer *buf, IrBasicBlock *bb)
{
    SNPRINTF(buf, "bb_%d:\n", bb->index);
    for (IrInstruction *instr = bb->head; instr != NULL; instr = instr->next)
    {
        instruction(buf, instr);
    }
}

static void function(CharBuffer *buf, IrFunction *func)
{
    SNPRINTF(buf, "void _%s(void)\n{\n", func->name);
    SNPRINTF(buf, INDENT "uint8_t sp[%d];\n", func->stack_size);

    // Declare all registers used within this function.
    for(int i = 0;i < func->register_count;i++)
    {
        SNPRINTF(buf, INDENT "uint32_t t%d;\n", i);
    }

    for (IrBasicBlock *bb = func->head; bb != NULL; bb = bb->next)
    {
        basic_block(buf, bb);
    }

    SNPRINTF(buf, "}\n");
}

static void program(CharBuffer *buf, IrProgram *prog)
{
    SNPRINTF(buf, HEADER);

    // First, declare all registers used within the program.
    for (int i = 0; i < prog->register_count.arg; i++)
    {
        SNPRINTF(buf, "uint32_t a%d;\n", i);
    }
    SNPRINTF(buf, "uint32_t r0;\n\n");

    // Now print out all the functions.
    for (IrFunction *func = prog->functions; func != NULL; func = func->next)
    {
        function(buf, func);
    }

    // Add a main function!
    SNPRINTF(buf, "int main(int argc, char ** argv){\n");
    SNPRINTF(buf, INDENT "_main();\n");
    SNPRINTF(buf, INDENT "return r0;\n}\n");
}

char *Ir_to_str(IrProgram *ir)
{
    char *chbuffer = calloc(4096, sizeof(char));
    CharBuffer buffer = {chbuffer, chbuffer + 4096};

    program(&buffer, ir);

    return chbuffer;
}