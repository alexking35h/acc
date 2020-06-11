#include <stdlib.h>

#include "ir.h"

#define EMIT(...) emit((IrInstruction){__VA_ARGS__})

typedef struct IrGenerator {
    IrProgram * program;
    IrFunction * current_function;
    IrBasicBlock * current_basic_block;
} IrGenerator;

/*
 * Helper functions
 */
static IrRegister * get_register(IrGenerator * irgen) {

}
static IrObject * allocate_local(IrGenerator * irgen, int size, int align, char * name) {
    IrObject * new = calloc(1, sizeof(IrObject));
    new->name = new;
    new->size = 1;
    new->alignment = 1;

    IrObject ** ptr;
    for(ptr = &(irgen->current_function->locals);*ptr;ptr = &(*ptr)->next);
    *ptr = new;
}
static IrObject * allocate_global(IrGenerator * irgen, int size, int align, char * name) {
    IrObject * new = calloc(1, sizeof(IrObject));
    new->name = name;
    new->size = 1;
    new->alignment = 1;

    IrObject ** ptr;
    for(ptr = &(irgen->program->globals);*ptr;ptr = &(*ptr)->next);
    *ptr = new;
}
static IrBasicBlock * new_basic_block(IrGenerator * irgen) {

}
static void emit(IrInstruction instr) {

}

IrRegister * walk_expr_binary(IrGenerator * irgen, ExprAstNode * node) {

}
IrRegister * walk_expr_unary(IrGenerator * irgen, ExprAstNode * node) {

}
IrRegister * walk_expr_primary(IrGenerator * irgen, ExprAstNode * node) {

}
IrRegister * walk_expr_postfix(IrGenerator * irgen, ExprAstNode * node) {

}
IrRegister * walk_expr_cast(IrGenerator * irgen, ExprAstNode * node) {

}
IrRegister * walk_expr_tertiary(IrGenerator * irgen, ExprAstNode * node) {

}
IrRegister * walk_expr_assign(IrGenerator * irgen, ExprAstNode * node) {

}

IrRegister * walk_expr(IrGenerator * irgen, ExprAstNode * node) {
    switch(node->type) {
        case BINARY:
            return walk_expr_binary(irgen, node);
        case UNARY:
            return walk_expr_unary(irgen, node);
        case PRIMARY:
            return walk_expr_primary(irgen, node);
        case POSTFIX:
            return walk_expr_postfix(irgen, node);
        case CAST:
            return walk_expr_cast(irgen, node);
        case TERTIARY:
            return walk_expr_tertiary(irgen, node);
        case ASSIGN:
            return walk_expr_assign(irgen, node);
    }
}

void walk_decl_function(IrGenerator * irgen, DeclAstNode * node) {
    IrFunction * func = calloc(1, sizeof(IrFunction));

    func->name = node->identifier->lexeme;
    func->entry = new_basic_block(irgen);

    irgen->current_function = func;
    irgen->current_basic_block = func->entry;

    walk_stmt(irgen, node->body);

    irgen->current_function = NULL;
}

void walk_decl_object(IrGenerator * irgen, DeclAstNode * node) {
    if(irgen->current_function) {
        allocate_local(irgen, 1, 1, node->identifier->lexeme);
    } else {
        allocate_global(irgen, 1, 1, node->identifier->lexeme);
    }
}

void walk_decl(IrGenerator * irgen, DeclAstNode *node) {
    if(node->body) {
        // We are declaring a new function
        walk_decl_function(irgen, node);
    } else {
        // We are declaring an object.
        walk_decl_object(irgen, node);
    }

    if(node->next) {
        walk_decl(irgen, node->next);
    }
}

void walk_stmt_while(IrGenerator * irgen, StmtAstNode * node) {

}

void walk_stmt_return(IrGenerator * irgen, StmtAstNode * node) {

}

void walk_stmt(IrGenerator * irgen, StmtAstNode *node) {
    switch(node->type) {
        case DECL:
            walk_decl(irgen, node->decl.decl);
            break;
        case EXPR:
            walk_expr(irgen, node->expr.expr);
            break;
        case BLOCK:
            walk_stmt(irgen, node->block.head);
            break;
        case WHILE_LOOP:
            walk_stmt_while(irgen, node);
            break;
        case RETURN_JUMP:
            walk_stmt_return(irgen, node);
            break;
    }

    if(node->next) {
        walk_stmt(irgen, node->next);
    }
}

IrProgram * Ir_generate(DeclAstNode * ast_root) {
    IrGenerator irgen = {
        .program = calloc(1, sizeof(IrProgram))
    };

    walk_decl(&irgen, ast_root);
    return irgen.program;
}

static Ir_to_str_instruction(IrInstruction * instr, char * buffer, int len) {
}

static int Ir_to_str_function(IrFunction * function, char * buffer, int len) {
    int l = snprintf(buffer, len, ".fun %s:\n", function->name);
    buffer += l;
    len -= l;

    // Print out all locals.
    for(IrObject * obj = function->locals;obj;obj = obj->next) {
        l = snprintf(buffer, len, "  .%s:%d,%d\n", obj->name, obj->size, obj->alignment);
        buffer += l;
        len -= l;
    }

    // Print out all basic blocks in the order that we see them
    IrBasicBlock * queue[64] = {function->entry};
    int block_head = 0, block_tail = 1;

    while(block_head != block_tail % 64) {
        // Print label, and print instruction.
        l = snprintf(buffer, len, ".block %s:\n", queue[block_head]->label);

        for(IrInstruction * instr = queue[block_head]->head; instr; instr = instr->next) {
            l = Ir_to_str_instruction(instr, buffer, len);
            buffer += l;
            len -= l;

            // Check for other blocks we need to follow
            if(instr->left.jump) {
                queue[block_tail++ % 64] = instr->left.jump;
            }
            if (instr->right.jump) {
                queue[block_tail++ % 64] = instr->right.jump;
            }
            block_head++;
        }
    }
}

char * Ir_to_str(IrProgram * ir) {

    char *buffer = calloc(1, 1024), *buf = buffer;
    int len = 1024;

    // Print out all global variables.
    for(IrObject * obj = ir->globals;obj;obj = obj->next) {
        int l = snprintf(buffer, len, ".%s:%d,%d\n", obj->name, obj->size, obj->alignment);
        buffer += l;
        len -= l;
    }

    // Print out all functions.
    for(IrFunction * fun = ir->functions;fun;fun = fun->next) {
        int l = Ir_to_str_function(fun, buffer, len);
        buffer += l;
        len -= l;
    }
    return buf;
}