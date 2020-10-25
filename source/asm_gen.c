#include <stdlib.h>

#include "ir.h"

#define HEADER                \
    "# == ACC ASSEMBLY == \n" \
    "#\n"                       \
    "# Date: " __DATE__ "\n"  \
    "# Time: " __TIME__ "\n\n"  \

#define INDENT "    "

/*
 * Arithmetic instructions:
 * - IR_ADD
 * - IR_SUB
 * - IR_MUL
 * - IR_MOD
 * - IR_SLL
 * - IR_SLR
 * - IR_OR
 * - IR_AND
 * - IR_NOT
 * - IR_FLIP
 * - IR_XOR
 */
static void arithmetic(FILE * fd, IrInstruction * instr)
{
    char * op;
    switch(instr->op) {
        case IR_ADD:
            op = "add";
            break;
        case IR_SUB:
            op = "sub";
            break;
        case IR_MUL:
        case IR_DIV:
        case IR_MOD:
        case IR_SLL:
        case IR_SLR:
        case IR_OR:
        case IR_AND:
        case IR_NOT:
        case IR_FLIP:
        case IR_XOR:
            abort();
    }
    fprintf(fd, "%s r%d, r%d, r%d\n", op, instr->dest->index, instr->left->index, instr->right->index);
}

/*
 * Comparison instructions:
 * - IR_EQ
 * - IR_LT
 * - IR_LE
 */
static void comparison(FILE * fd, IrInstruction * instr)
{

}

/* 
 * Sign-extend instructions:
 * - IR_SIGN_EXTEND_8
 * - IR_SIGN_EXTEND_16
 */
static void sign_extend(FILE * fd, IrInstruction * instr)
{

}

/*
 * IR_MOV instruction
 */
static void move(FILE * fd, IrInstruction * instr)
{
    fprintf(fd, INDENT "mov r%d, r%d\n", instr->dest->index, instr->left->index);
}

/*
 * Store instructions
 * - IR_STORE8
 * - IR_STORE16
 * - IR_STORE32
 */
static void store(FILE * fd, IrInstruction * instr)
{

}

/*
 * Load instructions
 * - IR_LOAD8
 * - IR_LOAD16
 * - IR_LOAD32
 */
static void load(FILE * fd, IrInstruction * instr)
{

}

/*
 * IR_LOADI instruction
 */
static void loadi(FILE * fd, IrInstruction * instr)
{
    fprintf(fd, INDENT "mov r%d, #%d\n", instr->dest->index, instr->value);
}

/*
 * IR_LOADSO instruction
 */
static void loadso(FILE * fd, IrInstruction * instr)
{

}

/* 
 * Control instructions:
 * - IR_BRANCHZ
 * - IR_JUMP
 * - IR_CALL
 * - IR_RETURN
 */
static void control(FILE * fd, IrInstruction * instr)
{

}

/*
 * Single IR instruction
 */
static void instruction(FILE * fd, IrInstruction * instr)
{

}

/*
 * Single basic block (including label)
 */
static void basic_block(FILE * fd, IrBasicBlock * bb)
{
    for(IrInstruction * instr = bb->head;instr != NULL;instr = instr->next)
    {
        switch(instr->op)
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
            case IR_FLIP:
            case IR_XOR:
                arithmetic(fd, instr);
                break;

            case IR_EQ:
            case IR_LT:
            case IR_LE:
                abort();

            case IR_SIGN_EXTEND_8:
            case IR_SIGN_EXTEND_16:
                abort();

            case IR_MOV:
                move(fd, instr);
                break;

            case IR_STORE8:
            case IR_STORE16:
            case IR_STORE32:
                abort();

            case IR_LOAD8:
            case IR_LOAD16:
            case IR_LOAD32:
                abort();

            case IR_LOADI:
                loadi(fd, instr);
                break;

            case IR_LOADSO:
                abort();

            case IR_BRANCHZ:
            case IR_JUMP:
            case IR_CALL:
            case IR_RETURN:
            case IR_NOP:
                fprintf(fd, INDENT "nop\n");
                break;
        }
    }
}

/*
 * Single function (including entry-label)
 */
static void function(FILE * fd, IrFunction * function)
{
    fprintf(fd, "\n");
    fprintf(fd, "%s:\n", function->name);

    // Function preamble:
    // Store all registers except r0, r1, r2, r3.
    fprintf(fd, INDENT "push {r4,r5,r6,r7,r8,r9,r10,r11,lr}\n");

    // Decrement the stack pointer.
    fprintf(fd, INDENT "sub sp, sp, #%u\n", function->stack_size);

    for(IrBasicBlock * bb = function->head;bb != NULL;bb = bb->next)
    {
        basic_block(fd, bb);
    }

    // Increment the stack pointer.
    fprintf(fd, INDENT "add sp, sp, #%u\n", function->stack_size);

    // Function postamble.
    // Pop all registers except r0, r1, r2, r3 off the stack and branch.
    fprintf(fd, INDENT "pop {r4,r5,r6,r7,r8,r9,r10,r11,lr}\n");
    fprintf(fd, INDENT "bx lr\n");
}

/*
 * _start function
 */
static void _start(FILE * fd)
{
    fprintf(fd, "_start:\n");
    fprintf(fd, INDENT "bl main\n");

    // Exit.
    fprintf(fd, INDENT "mov r7, #1\n");
    fprintf(fd, INDENT "svc #0\n");
}

void assembly_gen(FILE * fd, IrFunction * program)
{
    fprintf(fd, HEADER);
    fprintf(fd, INDENT ".global _start\n");
    fprintf(fd, INDENT ".text\n\n");

    for(IrFunction * f = program;f != NULL;f = f->next)
    {
        fprintf(fd, INDENT ".global %s\n", f->name);
    }

    _start(fd);

    for(;program != NULL;program = program->next)
    {
        function(fd, program);
    }
}