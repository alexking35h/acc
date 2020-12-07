#ifndef __IR___
#define __IR___

#include <stdbool.h>
#include <stdint.h>
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
    IR_FLIP,
    IR_XOR,

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

    // Load-stack offset
    IR_LOADSO,

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
    REG_RESERVED,
    REG_ANY,
    REG_SPILL
} IrRegType;

typedef struct IrRegister
{
    IrRegType type;

    union {
        int index;
        int spill;
    };

    struct
    {
        unsigned int start;
        unsigned int finish;
    } liveness;

    struct IrRegister *next;
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

    struct {
        IrBasicBlock * jump_true;
        IrBasicBlock * jump_false;
        union
        {
            IrFunction * callee;
            char * callee_name;
        };
        
    } control;

    int live_position;

    struct IrInstruction *next, *prev;
} IrInstruction;

typedef struct IrBasicBlock
{
    int index;

    struct 
    {
        uint8_t * entry;
        uint8_t * exit;
    } live;

    IrBasicBlock * cfg_entry[2];

    IrInstruction *head, *tail;
    IrBasicBlock *next;
} IrBasicBlock;

typedef struct IrFunction
{
    char *name;
    int stack_size;

    struct 
    {
        IrRegister **list;
        int list_size;
        int count;
    } registers;

    bool has_regalloc;
    int regalloc_count;

    IrBasicBlock *head, *tail;
    struct IrFunction *next;
} IrFunction;

/*
 * Generate string-representation of the IR.
 */
void Ir_to_str(FILE *, IrFunction *, int *);

/*
 * Append an instruction to a basic block.
 */
void Ir_emit_instr(IrBasicBlock * bb, IrInstruction instr);

/*
 * Insert a new instruction before/after another instruction.
 */
void Ir_emit_instr_after(IrInstruction * after, IrInstruction * instr);
void Ir_emit_instr_before(IrInstruction * before, IrInstruction * instr);

#endif