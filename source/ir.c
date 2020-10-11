#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "ir.h"

#define INDENT "    "

#define HEADER                                                                           \
    "// === ACC IR === \n//\n"                                                           \
    "// Date: " __DATE__ "\n"                                                            \
    "// Time: " __TIME__ "\n\n"                                                          \
    "typedef unsigned int uint32_t;\n"                                                   \
    "typedef unsigned short uint16_t;\n"                                                 \
    "typedef unsigned char uint8_t;\n"                                                   \
    "typedef signed int int32_t;\n"                                                      \
    "typedef signed short int16_t;\n"                                                    \
    "typedef signed char int8_t;\n\n"                                                    \
    "#if __INT_WIDTH__ != __INTPTR_WIDTH__\n"                                            \
    "#error Require 32-bit system (int32 and pointers should have the same size)\n"      \
    "#endif\n\n"                                                                         \
    "#define SIGN_EXTEND8(c) (c | (c & 0x80 ? 0xFFFFFF00 : 0))\n"                        \
    "#define SIGN_EXTEND16(c) (c | (c & 0x8000 ? 0xFFFF0000 : 0))\n\n"

static void ir_register(FILE *fd, IrRegister *reg)
{
    switch (reg->type)
    {
    case REG_RESERVED:
        fprintf(fd, "r%d", reg->virtual_index);
        break;
    case REG_ANY:
        fprintf(fd, "t%d", reg->virtual_index);
        break;
    }
}

static void instruction_arithmetic(FILE *fd, IrInstruction *instr)
{
    fprintf(fd, INDENT);
    ir_register(fd, instr->dest);
    fprintf(fd, " = ");

    if (instr->right)
    {
        ir_register(fd, instr->left);
    }
    switch (instr->op)
    {
    case IR_ADD:
        fprintf(fd, " + ");
        break;
    case IR_SUB:
        fprintf(fd, " - ");
        break;
    case IR_MUL:
        fprintf(fd, " * ");
        break;
    case IR_DIV:
        fprintf(fd, " / ");
        break;
    case IR_MOD:
        fprintf(fd, " %% ");
        break;
    case IR_SLL:
        fprintf(fd, " << ");
        break;
    case IR_SLR:
        fprintf(fd, " >> ");
        break;
    case IR_OR:
        fprintf(fd, " | ");
        break;
    case IR_AND:
        fprintf(fd, " & ");
        break;
    case IR_NOT:
        fprintf(fd, " ! ");
        break;
    case IR_FLIP:
        fprintf(fd, " ~ ");
        break;
    case IR_XOR:
        fprintf(fd, " ^ ");
        break;
    case IR_EQ:
        fprintf(fd, " == ");
        break;
    case IR_LT:
        fprintf(fd, " < ");
        break;
    case IR_LE:
        fprintf(fd, " <= ");
        break;
    }

    if (instr->right)
    {
        ir_register(fd, instr->right);
    }
    else
    {
        ir_register(fd, instr->left);
    }
    fprintf(fd, ";\n");
}

static void instruction_sign_extend(FILE *fd, IrInstruction *instr)
{
    fprintf(fd, INDENT);
    ir_register(fd, instr->dest);
    if (instr->op == IR_SIGN_EXTEND_8)
    {
        fprintf(fd, " = SIGN_EXTEND8(");
    }
    else
    {
        fprintf(fd, " = SIGN_EXTEND16(");
    }
    ir_register(fd, instr->left);
    fprintf(fd, ");\n");
}

static void instruction_move(FILE *fd, IrInstruction *instr)
{
    fprintf(fd, INDENT);
    ir_register(fd, instr->dest);
    fprintf(fd, " = ");
    ir_register(fd, instr->left);
    fprintf(fd, ";\n");
}

static void instruction_mem(FILE *fd, IrInstruction *instr)
{
    fprintf(fd, INDENT);
    if (instr->op == IR_LOAD8 || instr->op == IR_LOAD16 || instr->op == IR_LOAD32)
    {
        ir_register(fd, instr->dest);
        switch (instr->op)
        {
        case IR_LOAD8:
            fprintf(fd, " = *((uint8_t*)");
            break;
        case IR_LOAD16:
            fprintf(fd, " = *((uint16_t*)");
            break;
        case IR_LOAD32:
            fprintf(fd, " = *((uint32_t*)");
            break;
        }
        ir_register(fd, instr->left);
        fprintf(fd, ")");
    }
    else
    {
        switch (instr->op)
        {
        case IR_STORE8:
            fprintf(fd, "*((uint8_t*)");
            break;
        case IR_STORE16:
            fprintf(fd, "*((uint16_t*)");
            break;
        case IR_STORE32:
            fprintf(fd, "*((uint32_t*)");
            break;
        }
        ir_register(fd, instr->left);
        fprintf(fd, ") = ");
        ir_register(fd, instr->right);
    }
    fprintf(fd, ";\n");
}

static void instruction_loadi(FILE *fd, IrInstruction *instr)
{
    fprintf(fd, INDENT);
    ir_register(fd, instr->dest);
    fprintf(fd, " = %d;\n", instr->value);
}

static void instruction_loadso(FILE *fd, IrInstruction *instr)
{
    fprintf(fd, INDENT);
    ir_register(fd, instr->dest);
    fprintf(fd, " = (uint32_t)sp + %d;\n", instr->value);
}

static void instruction_jump(FILE *fd, IrInstruction *instr)
{
    if (instr->op == IR_JUMP)
    {
        fprintf(fd, INDENT "goto bb_%d;\n", instr->control.jump_true->index);
    }
    else if (instr->op == IR_RETURN)
    {
        fprintf(fd, INDENT "return;\n");
    }
    else if (instr->op == IR_BRANCHZ)
    {
        fprintf(fd, INDENT "if(");
        ir_register(fd, instr->left);
        fprintf(fd, ")\n" INDENT "{\n");
        fprintf(fd, INDENT INDENT "goto bb_%d;\n", instr->control.jump_true->index);
        fprintf(fd, INDENT "} else {\n");
        fprintf(fd, INDENT INDENT "goto bb_%d;\n", instr->control.jump_false->index);
        fprintf(fd, INDENT "}\n");
    }
    else if (instr->op == IR_CALL)
    {
        fprintf(fd, INDENT "_%s();\n", instr->control.callee->name);
    }
}

static void instruction(FILE *fd, IrInstruction *instr)
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
    case IR_XOR:
    case IR_FLIP:
    case IR_EQ:
    case IR_LT:
    case IR_LE:
        instruction_arithmetic(fd, instr);
        break;

    case IR_SIGN_EXTEND_16:
    case IR_SIGN_EXTEND_8:
        instruction_sign_extend(fd, instr);
        break;

    case IR_MOV:
        instruction_move(fd, instr);
        break;

    case IR_STORE8:
    case IR_STORE16:
    case IR_STORE32:
    case IR_LOAD8:
    case IR_LOAD16:
    case IR_LOAD32:
        instruction_mem(fd, instr);
        break;

    case IR_LOADI:
        instruction_loadi(fd, instr);
        break;
    
    case IR_LOADSO:
        instruction_loadso(fd, instr);
        break;

    case IR_BRANCHZ:
    case IR_JUMP:
    case IR_CALL:
    case IR_RETURN:
        instruction_jump(fd, instr);
        break;
    case IR_NOP:
        fprintf(fd, INDENT ";\n");
    }
}

static void basic_block(FILE *fd, IrBasicBlock *bb)
{
    fprintf(fd, "bb_%d:\n", bb->index);
    for (IrInstruction *instr = bb->head; instr != NULL; instr = instr->next)
    {
        instruction(fd, instr);
    }
}

static void function(FILE *fd, IrFunction *func)
{
    fprintf(fd, "void _%s(void)\n{\n", func->name);
    fprintf(fd, INDENT "_Alignas(4) uint8_t sp[%d];\n", func->stack_size);

    // Declare all registers used within this function.
    for (int i = 0; i < func->registers.count; i++)
    {
        fprintf(fd, INDENT "uint32_t t%d;\n", i);
    }

    for (IrBasicBlock *bb = func->head; bb != NULL; bb = bb->next)
    {
        basic_block(fd, bb);
    }

    fprintf(fd, "}\n");
}

void Ir_to_str(IrFunction *ir, FILE *fd)
{
    fprintf(fd, HEADER);

    // Declare all registers used within the program
    for (int i = 0; i < 3; i++)
    {
        fprintf(fd, "uint32_t r%d = 0;\n", i);
    }

    // Print out all functions.
    for (IrFunction *func = ir;func != NULL;func = func->next)
    {
        function(fd, func);
    }

    // Add a main function/entry point.
    fprintf(fd, "int main(int argc, char ** argv){\n");
    fprintf(fd, INDENT "_main();\n");
    fprintf(fd, INDENT "return r0;\n}\n");
}

void Ir_emit_instr(IrBasicBlock * bb, IrInstruction instr)
{
    IrInstruction * new_instr = calloc(1, sizeof(IrInstruction));
    memcpy(new_instr, &instr, sizeof(IrInstruction));

    if(bb->head)
    {
        new_instr->prev = bb->tail;
        bb->tail->next = new_instr;
        bb->tail = new_instr;
    } else {
        bb->head = bb->tail = new_instr;
    }
}

void Ir_emit_instr_after(IrInstruction * prev, IrInstruction * instr)
{
    instr->prev = prev;
    instr->next = prev->next;

    prev->next->prev = instr;
    prev->next = instr;
}

void Ir_emit_instr_before(IrInstruction * after, IrInstruction * instr)
{
    instr->next = after;
    instr->prev = after->prev;

    after->prev->next = instr;
    after->prev = instr;
}