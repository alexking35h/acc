#ifndef __IR___
#define __IR___

typedef struct IrBasicBlock IrBasicBlock;
typedef struct IrObject IrObject;

typedef enum IrOpcode
{
    // Arithmetic instructions
    // dest = left OP right
    ADD,
    SUB,
    MUL,
    DIV,
    MOD,

    // Move operation
    // dest = left
    MOV,

    // Store
    // memory @ left + right = dest register.
    STORE,

    // Load
    // load memory @ left + right into dest register.
    LOAD,

    // Load-immediate. Immediate can be integer, or local-offset.
    // dest = immediate value
    LOADI,

    // Cast
    // dest = cast (left)
    TYPE_CAST,

    // Call & return
    // Call takes any number of argument registers, which are set in the context of the
    // callee.
    // Return stores the state of the 'R' register.
    CALL,
    RET,

} IrOpcode;

typedef struct IrRegister
{
    enum {
        REGISTER_VALUE,
        REGISTER_ADDRESS
    } type;
    
    int index;
} IrRegister;

typedef struct IrInstruction
{
    IrOpcode op;

    // Add, Sub, Mul, Div, Mod, Mov, Store, Load
    IrRegister * dest;
    IrRegister *left;
    IrRegister *right;

    struct {
        enum {
            IMMEDIATE_VALUE = 1,
            IMMEDIATE_OBJECT
        } type;
        IrObject * object;
        int value;
    } immediate;

    IrBasicBlock * jump_true;
    IrBasicBlock * jump_false;

    struct IrInstruction *next;
} IrInstruction;

typedef struct IrBasicBlock
{
    char *label;
    IrInstruction *head;
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
char * Ir_to_str(IrProgram *);

#endif