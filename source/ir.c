#include <stdio.h>
#include <stdlib.h>

#include "ir.h"

#define SNPRINTF(cb, ...) (cb)->pos += snprintf((cb)->buffer + (cb)->pos, (cb)->len - (cb)->pos, __VA_ARGS__)

typedef struct CharBuffer {
    char * buffer;
    int len;
    int pos;
} CharBuffer;

static char * Ir_to_str_op(IrOpcode op) {
    switch(op) {
        case ADD: return "add";
        case SUB: return "sub";
        case MUL: return "mul";
        case DIV: return "div";
        case MOD: return "mod";
        case MOV: return "mov";
        case STORE: return "store";
        case LOAD: return "load";
        case LOADI: return "loadi";
        case TYPE_CAST: return "cast";
        case CALL: return "call";
        case RET: return "return";
    }
}

static void Ir_to_str_reg(CharBuffer * buffer, IrRegister * reg) {
    SNPRINTF(buffer, "r%d, ", reg->index);
}

static void Ir_to_str_instruction(CharBuffer * buffer, IrInstruction * instr) {
    SNPRINTF(buffer, "    %s, ", Ir_to_str_op(instr->op));

    if(instr->dest) {
        Ir_to_str_reg(buffer, instr->dest);
    }

    if(instr->left) {
        Ir_to_str_reg(buffer, instr->left);
    }

    if(instr->right) {
        Ir_to_str_reg(buffer, instr->right);
    }

    if(instr->immediate.type == IMMEDIATE_VALUE) {
        SNPRINTF(buffer, "%d, ", instr->immediate.value);
    } else if (instr->immediate.type == IMMEDIATE_OBJECT) {
        SNPRINTF(buffer, "@%s, ", instr->immediate.object->name);
    }
    SNPRINTF(buffer, "\n");
}

void Ir_to_str_function(CharBuffer * buffer, IrFunction * function) {
    SNPRINTF(buffer, ".fun %s:\n", function->name);

    // Print out all locals.
    for(IrObject * obj = function->locals;obj;obj = obj->next) {
        SNPRINTF(buffer, "  .local %s:%d,%d\n", obj->name, obj->size, obj->alignment);
    }

    // Print out all basic blocks in the order that we see them
    IrBasicBlock * queue[64] = {function->entry};
    int block_head = 0, block_tail = 1;

    while((block_head % 64) != (block_tail % 64)) {
        // Print label, and print instruction.
        SNPRINTF(buffer, "  .block %s:\n", queue[block_head]->label);

        for(IrInstruction * instr = queue[block_head]->head; NULL != instr; instr = instr->next) {
            Ir_to_str_instruction(buffer, instr);

            // // Check for other blocks we need to follow
            // if(instr->left.jump) {
            //     queue[block_tail++ % 64] = instr->left.jump;
            // }
            // if (instr->right.jump) {
            //     queue[block_tail++ % 64] = instr->right.jump;
            // }
        }
        block_head++;
    }
}



char * Ir_to_str(IrProgram * ir) {

    CharBuffer buffer = {
        .buffer = calloc(1, 1024),
        .pos = 0,
        .len = 1024,
    };

    // Print out all global variables.
    for(IrObject * obj = ir->globals;obj;obj = obj->next) {
        SNPRINTF(&buffer,
            ".global %s:%d,%d\n",
            obj->name,
            obj->size,
            obj->alignment
        );
    }

    // Print out all functions.
    for(IrFunction * fun = ir->functions;fun;fun = fun->next) {
        Ir_to_str_function(&buffer, fun);
    }
    return buffer.buffer;
}