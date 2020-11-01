#include <stdlib.h>

#include "ir.h"

#define HEADER                \
    "# == ACC ASSEMBLY == \n" \
    "#\n"                       \
    "# Date: " __DATE__ "\n"  \
    "# Time: " __TIME__ "\n\n"  \

#define INDENT "    "

static void function_enter(FILE * fd, IrFunction * function)
{
    // Store all registers except r0, r1, r2, r3.
    fprintf(fd, INDENT "push {r4,r5,r6,r7,r8,r9,r10,r11,lr}\n");

    // Decrement the stack pointer.
    fprintf(fd, INDENT "sub sp, sp, #%u\n", function->stack_size);
}

static void function_exit(FILE * fd, IrFunction * function)
{
    // Increment the stack pointer.
    fprintf(fd, INDENT "add sp, sp, #%u\n", function->stack_size);

    // Function postamble.
    // Pop all registers except r0, r1, r2, r3 off the stack and branch.
    fprintf(fd, INDENT "pop {r4,r5,r6,r7,r8,r9,r10,r11,lr}\n");
    fprintf(fd, INDENT "bx lr\n");
}

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
            op = "mul";
            break;
        case IR_DIV:
            op = "sdiv";
            break;
        case IR_MOD:
            fprintf(fd, INDENT "sdiv r%d, r%d, r%d\n", instr->dest->index, instr->left->index, instr->right->index);
            fprintf(fd, INDENT "mul r%d, r%d, r%d\n", instr->dest->index, instr->dest->index, instr->right->index);
            fprintf(fd, INDENT "sub r%d, r%d, r%d\n", instr->dest->index, instr->left->index, instr->dest->index);
            return;
        case IR_SLL:
            op = "lsl";
            break;

        case IR_SLR:
            op = "lsr";
            break;

        case IR_OR:
            op = "orr";
            break;

        case IR_AND:
            op = "and";
            break;

        case IR_NOT:
            fprintf(fd, INDENT "cmp r%d, #0\n", instr->left->index);
            fprintf(fd, INDENT "moveq r%d, #1\n", instr->dest->index);
            fprintf(fd, INDENT "movne r%d, #0\n", instr->dest->index);
            return;

        case IR_FLIP:
            fprintf(fd, INDENT "mvn r%d, #0\n", instr->dest->index);
            fprintf(fd, INDENT "eor r%d, r%d, r%d\n", instr->dest->index, instr->dest->index, instr->left->index);
            return;

        case IR_XOR:
            op = "eor";
            break;
    }
    fprintf(fd, INDENT "%s r%d, r%d, r%d\n", op, instr->dest->index, instr->left->index, instr->right->index);
}

/*
 * Comparison instructions:
 * - IR_EQ
 * - IR_LT
 * - IR_LE
 */
static void comparison(FILE * fd, IrInstruction * instr)
{
    fprintf(fd, INDENT "cmp r%d, r%d\n", instr->left->index, instr->right->index);
    switch(instr->op)
    {
        case IR_EQ:
            fprintf(fd, INDENT "moveq r%d, #1\n", instr->dest->index);
            fprintf(fd, INDENT "movne r%d, #0\n", instr->dest->index);
            break;
        
        case IR_LT:
            fprintf(fd, INDENT "movlt r%d, #1\n", instr->dest->index);
            fprintf(fd, INDENT "movge r%d, #0\n", instr->dest->index);
            break;
        
        case IR_LE:
            fprintf(fd, INDENT "movle r%d, #1\n", instr->dest->index);
            fprintf(fd, INDENT "movgt r%d, #0\n", instr->dest->index);
            break;
    }
}

/* 
 * Sign-extend instructions:
 * - IR_SIGN_EXTEND_8
 * - IR_SIGN_EXTEND_16
 */
static void sign_extend(FILE * fd, IrInstruction * instr)
{
    switch(instr->op)
    {
        case IR_SIGN_EXTEND_16:
            fprintf(fd, INDENT "sxth r%d, r%d\n", instr->dest->index, instr->left->index);
            break;
        case IR_SIGN_EXTEND_8:
            fprintf(fd, INDENT "sxtb r%d, r%d\n", instr->dest->index, instr->left->index);
            break;
    }
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
    switch(instr->op)
    {
        case IR_STORE32:
            fprintf(fd, INDENT "str r%d, [r%d]\n", instr->right->index, instr->left->index);
            break;
        case IR_STORE16:
            fprintf(fd, INDENT "strh r%d, [r%d]\n", instr->right->index, instr->left->index);
            break;
        case IR_STORE8:
            fprintf(fd, INDENT "strb r%d, [r%d]\n", instr->right->index, instr->left->index);
            break;
    }
}

/*
 * Load instructions
 * - IR_LOAD8
 * - IR_LOAD16
 * - IR_LOAD32
 */
static void load(FILE * fd, IrInstruction * instr)
{
    switch(instr->op)
    {
        case IR_LOAD32:
            fprintf(fd, INDENT "ldr r%d, [r%d]\n", instr->dest->index, instr->left->index);
            break;
        case IR_LOAD16:
            fprintf(fd, INDENT "ldrh r%d, [r%d]\n", instr->dest->index, instr->left->index);
            break;
        case IR_LOAD8:
            fprintf(fd, INDENT "ldrb r%d, [r%d]\n", instr->dest->index, instr->left->index);
            break;
    }
}

/*
 * IR_LOADI instruction
 */
static void loadi(FILE * fd, IrInstruction * instr)
{
    if(instr->value > 0xFFFF)
    {
        fprintf(fd, INDENT "mov r%d, #%d\n", instr->dest->index, instr->value >> 16);
        fprintf(fd, INDENT "lsl r%d, r%d, #16\n", instr->dest->index, instr->dest->index);
        fprintf(fd, INDENT "orr r%d, #%d\n", instr->dest->index, instr->value & 0xFFFF);
    } 
    else
    {
        fprintf(fd, INDENT "mov r%d, #%d\n", instr->dest->index, instr->value);
    }
}

/*
 * IR_LOADSO instruction
 */
static void loadso(FILE * fd, IrInstruction * instr)
{
    fprintf(fd, INDENT "add r%d, sp, #%d\n", instr->dest->index, instr->value);
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
    switch(instr->op)
    {
        case IR_BRANCHZ:
            fprintf(fd, INDENT "cmp r%d, #0\n", instr->left->index);
            fprintf(fd, INDENT "bne _bb_%d\n", instr->control.jump_true->index);
            fprintf(fd, INDENT "b _bb_%d\n", instr->control.jump_false->index);
            break;
        
        case IR_JUMP:
            fprintf(fd, INDENT "b _bb_%d\n", instr->control.jump_true->index);
            break;

        case IR_CALL:
            fprintf(fd, INDENT "bl %s\n", instr->control.callee->name);
            break;
    }
}

/*
 * Single basic block (including label)
 */
static void basic_block(FILE * fd, IrFunction * function, IrBasicBlock * bb)
{
    fprintf(fd, "_bb_%d:\n", bb->index);
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
                comparison(fd, instr);
                break;

            case IR_SIGN_EXTEND_8:
            case IR_SIGN_EXTEND_16:
                sign_extend(fd, instr);

            case IR_MOV:
                move(fd, instr);
                break;

            case IR_STORE8:
            case IR_STORE16:
            case IR_STORE32:
                store(fd, instr);
                break;

            case IR_LOAD8:
            case IR_LOAD16:
            case IR_LOAD32:
                load(fd, instr);
                break;

            case IR_LOADI:
                loadi(fd, instr);
                break;

            case IR_LOADSO:
                loadso(fd, instr);

            case IR_BRANCHZ:
            case IR_JUMP:
            case IR_CALL:
                control(fd, instr);
                break;

            case IR_RETURN:
                function_exit(fd, function);
                break;

            case IR_NOP:
                fprintf(fd, INDENT "nop;\n");
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
        basic_block(fd, function, bb);
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