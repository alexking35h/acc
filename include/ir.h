#ifndef __IR___
#define __IR___

typedef struct IrBasicBlock IrBasicBlock;
typedef struct IrObject IrObject;

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

    // Move instruction. dest = left
    IR_MOV,

    // Store instruction memory @ left = right
    IR_STORE,

    // Load instruction dest = memory @ left
    IR_LOAD,

    // Load-immediate (integer)
    IR_LOADI,

    // Load-address (of an object)
    IR_LOADA,

    // Branch-if-zero: true/false arms.
    // Unconditional jump.
    IR_BRANCHZ,
    IR_JUMP,

    // Call & Return
    IR_CALL,
    IR_RETURN

} IrOpcode;

typedef enum
{
    REG_ARGUMENT,
    REG_RETURN,
    REG_ANY
} IrRegType;

typedef struct IrRegister
{
    int index;
    IrRegType type;

} IrRegister;

typedef struct IrInstruction
{
    IrOpcode op;

    IrRegister *dest;
    IrRegister *left;
    IrRegister *right;

    // Loadi instruction
    int value;

    // Loada instruction
    IrObject *object;

    union {
        IrBasicBlock *jump;
        IrBasicBlock *jump_true;
    };
    IrBasicBlock *jump_false;

    struct IrInstruction *next;
} IrInstruction;

typedef struct IrBasicBlock
{
    char *label;
    IrInstruction *head;
    IrBasicBlock *next;
} IrBasicBlock;

typedef struct IrObject
{
    char *name;

    int size;
    int alignment;
    int index;

    struct IrObject *next;
} IrObject;

typedef struct IrFunction
{
    char *name;
    IrObject *locals;
    IrBasicBlock *entry;

    struct IrFunction *next;
} IrFunction;

typedef struct IrProgram
{
    IrObject *globals;
    IrFunction *functions;
} IrProgram;

/*
 * Generate string-representation of the IR.
 */
char *Ir_to_str(IrProgram *);

#endif