#ifndef __IR___
#define __IR___

#include <stdbool.h>
#include <stdio.h>

typedef struct IrBasicBlock IrBasicBlock;
typedef struct IrObject IrObject;
typedef struct IrFunction IrFunction;

typedef enum IrOpcode
{
    // Arithmetic instructions
    // dest = left OP right
    IR_ADD,
    IR_SUB,
    IR_MUL,
    IR_DIV,
    IR_MOD,
    IR_SLL,
    IR_SLR,
    IR_OR,
    IR_AND,
    IR_NOT,

    // Comparison - equal, less than, less than or equal
    // dest = 1 if left OP right, else 0
    IR_EQ,
    IR_LT,
    IR_LE,

    // Sign-extend operations.
    IR_SIGN_EXTEND_8,
    IR_SIGN_EXTEND_16,

    // Move instruction. dest = left
    IR_MOV,

    // Store instruction memory @ left = right
    IR_STORE8,
    IR_STORE16,
    IR_STORE32,

    // Load instruction dest = memory @ left
    IR_LOAD8,
    IR_LOAD16,
    IR_LOAD32,

    // Load-immediate (integer)
    IR_LOADI,

    // Stack operations (function preamble/postable)
    IR_STACK,
    IR_UNSTACK,

    // Branch-if-zero: true/false arms.
    // Unconditional jump.
    IR_BRANCHZ,
    IR_JUMP,

    // Call & Return
    IR_CALL,
    IR_RETURN,

    IR_NOP
} IrOpcode;

typedef enum
{
    REG_ARGUMENT,
    REG_RETURN,
    REG_ANY,
    REG_STACK
} IrRegType;

typedef struct IrRegister
{
    int index;
    IrRegType type;

} IrRegister;

typedef struct IrObject
{
    enum
    {
        LOCAL,
        GLOBAL
    } storage;

    int offset;
    int size;
    int align;
    bool sign;
} IrObject;

typedef struct IrInstruction
{
    IrOpcode op;

    IrRegister *dest;
    IrRegister *left;
    IrRegister *right;

    // Loadi instruction
    int value;

    union {
        IrBasicBlock *jump;
        IrBasicBlock *jump_true;
    };
    IrBasicBlock *jump_false;
    IrFunction *function;

    struct IrInstruction *next;
} IrInstruction;

typedef struct IrBasicBlock
{
    int index;
    IrInstruction *head, *tail;
    IrBasicBlock *next;
} IrBasicBlock;

typedef struct IrFunction
{
    char *name;
    int stack_size;
    IrBasicBlock *head, *tail;
    int register_count;

    struct IrFunction *next;
} IrFunction;

typedef struct IrProgram
{
    int bss_size;

    struct
    {
        int arg;
        int ret;
    } register_count;

    IrFunction *head, *tail;
} IrProgram;

/*
 * Generate string-representation of the IR.
 */
void Ir_to_str(IrProgram *, FILE *);

#endif