#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "arch.h"
#include "ir.h"
#include "ir_gen.h"

#define EMIT(irgen, opcode, ...)                                                         \
    Ir_emit_instr(irgen->current_basic_block, (IrInstruction){.op = opcode, __VA_ARGS__})
#define UPDATE_CFG(from, to)                                                             \
    *(to->cfg_entry[0] ? to->cfg_entry + 1 : to->cfg_entry) = from

typedef struct IrGenerator
{
    IrFunction *current_function;
    IrBasicBlock *current_basic_block;

    int bb_counter;
} IrGenerator;

static IrRegister *get_reg_any(IrGenerator *);
static IrRegister *get_reg_reserved(IrGenerator *, int);
static IrRegister *walk_expr(IrGenerator *, ExprAstNode *);
static void walk_stmt(IrGenerator *irgen, StmtAstNode *node);

static IrRegister * emit_loadi(IrGenerator * irgen, int value)
{
    IrRegister * dest = get_reg_any(irgen);
    EMIT(irgen, IR_LOADI, .dest=dest, .value=value);
    return dest;
}
static IrRegister * emit_loadso(IrGenerator * irgen, int value)
{
    IrRegister * dest = get_reg_any(irgen);
    EMIT(irgen, IR_LOADSO, .dest=dest, .value=value);
    return dest;
}
static IrRegister * emit_arith(IrGenerator * irgen, IrOpcode op, IrRegister * left, IrRegister * right)
{
    IrRegister * dest = get_reg_any(irgen);
    EMIT(irgen, op, .dest=dest, .left=left, .right=right);
    return dest;
}
static void emit_jump(IrGenerator *irgen, IrBasicBlock *b)
{
    UPDATE_CFG(irgen->current_basic_block, b);
    EMIT(irgen, IR_JUMP, .control.jump_true = b);
}
static void emit_jumpz(IrGenerator *irgen, IrBasicBlock *tb, IrBasicBlock *fb,
                       IrRegister *cond)
{
    UPDATE_CFG(irgen->current_basic_block, tb);
    UPDATE_CFG(irgen->current_basic_block, fb);
    EMIT(irgen, IR_BRANCHZ, .left = cond, .control.jump_true = tb, .control.jump_false = fb);
}
static void emit_store(IrGenerator *irgen, ExprAstNode *lhs, IrRegister *rhs)
{
    if (lhs->type == UNARY && lhs->unary.op == UNARY_DEREFERENCE)
    {
        IrRegister *dest = walk_expr(irgen, lhs->unary.right);
        if (lhs->unary.ptr_type->basic.type_specifier & TYPE_CHAR)
        {
            EMIT(irgen, IR_STORE8, .left = dest, .right = rhs);
        }
        else if (lhs->unary.ptr_type->basic.type_specifier & TYPE_SHORT)
        {
            EMIT(irgen, IR_STORE16, .left = dest, .right = rhs);
        }
        else
        {
            EMIT(irgen, IR_STORE32, .left = dest, .right = rhs);
        }
    }
    else if (lhs->type == PRIMARY && lhs->primary.symbol->ir.regster)
    {
        // Object is in a register.
        EMIT(irgen, IR_MOV, .dest = lhs->primary.symbol->ir.regster, .left = rhs);
    }
    else
    {
        abort();
    }
}
static IrRegister *emit_load(IrGenerator *irgen, ExprAstNode *src)
{
    IrRegister *dest = get_reg_any(irgen);

    if (src->unary.ptr_type->basic.type_specifier & TYPE_CHAR)
    {
        EMIT(irgen, IR_LOAD8, .dest = dest, .left = walk_expr(irgen, src->unary.right));
    }
    else if (src->unary.ptr_type->basic.type_specifier & TYPE_SHORT)
    {
        EMIT(irgen, IR_LOAD16, .dest = dest, .left = walk_expr(irgen, src->unary.right));
    }
    else
    {
        EMIT(irgen, IR_LOAD32, .dest = dest, .left = walk_expr(irgen, src->unary.right));
    }
    return dest;
}

static IrBasicBlock *new_bb(IrGenerator *irgen, IrFunction *function)
{
    IrBasicBlock *bb = calloc(1, sizeof(IrBasicBlock));
    bb->index = irgen->bb_counter++;
    Ir_emit_instr(bb, (IrInstruction){IR_NOP});

    if (function->head == NULL)
    {
        function->head = function->tail = bb;
    }
    else
    {
        function->tail->next = bb;
        function->tail = bb;
    }
    return bb;
}

static IrFunction *new_function(IrGenerator *irgen, char *name)
{
    IrFunction *function = calloc(1, sizeof(IrFunction));
    function->name = name;

    return function;
}

static IrRegister *get_reg_any(IrGenerator *irgen)
{
    int reg_idx = irgen->current_function->registers.count++;

    IrRegister *reg = calloc(1, sizeof(IrRegister));
    reg->type = REG_ANY;
    reg->index = reg_idx;
    reg->liveness.start = -1;
    reg->liveness.finish = 0;

    if(reg_idx >= irgen->current_function->registers.list_size)
    {
        irgen->current_function->registers.list = (IrRegister**)realloc(
            irgen->current_function->registers.list, 
            sizeof(IrRegister**) * (irgen->current_function->registers.list_size += 32)
        );
    }
    irgen->current_function->registers.list[reg_idx] = reg;

    return reg;
}

static IrRegister *get_reg_reserved(IrGenerator *irgen, int index)
{
    IrRegister *reg = calloc(1, sizeof(IrRegister));
    reg->type = REG_RESERVED;
    reg->index = index;

    return reg;
}

static IrObject *stack_allocate(IrFunction *function, Symbol *symbol)
{
    // Allocate space for a symbol on the stack.
    int size = arch_get_size(symbol->type);
    int align = arch_get_align(symbol->type);
    int sign = arch_get_signed(symbol->type);
    int offset = function->stack_size;

    // For now, just give everything on the stack 4 bytes.
    function->stack_size += (size + 3) & ~3;

    IrObject *object = calloc(1, sizeof(IrObject));
    object->size = size;
    object->align = align;
    object->sign = sign;
    object->offset = offset;
    object->storage = LOCAL;

    return object;
}

static IrRegister *walk_expr_binary(IrGenerator *irgen, ExprAstNode *node)
{
    IrOpcode op;
    bool post_op_negate = false;

    IrRegister *left = walk_expr(irgen, node->binary.left);
    IrRegister *right = walk_expr(irgen, node->binary.right);

    if (node->binary.ptr_scale_left != 0)
    {
        int scale = node->binary.ptr_scale_left;
        left = emit_arith(irgen, IR_MUL, left, emit_loadi(irgen, scale));
    }
    if (node->binary.ptr_scale_right != 0)
    {
        int scale = node->binary.ptr_scale_right;
        right = emit_arith(irgen, IR_MUL, right, emit_loadi(irgen, scale));
    }

    switch (node->binary.op)
    {
    case BINARY_ADD:
        op = IR_ADD;
        break;
    case BINARY_SUB:
        op = IR_SUB;
        break;
    case BINARY_MUL:
        op = IR_MUL;
        break;
    case BINARY_DIV:
        op = IR_DIV;
        break;
    case BINARY_MOD:
        op = IR_MOD;
        break;
    case BINARY_SLL:
        op = IR_SLL;
        break;
    case BINARY_SLR:
        op = IR_SLR;
        break;
    case BINARY_LT:
        op = IR_LT;
        break;
    case BINARY_GT:
        op = IR_LE;
        post_op_negate = true;
        break;
    case BINARY_LE:
        op = IR_LE;
        break;
    case BINARY_GE:
        op = IR_LT;
        post_op_negate = true;
        break;
    case BINARY_EQ:
        op = IR_EQ;
        break;
    case BINARY_NE:
        op = IR_EQ;
        post_op_negate = true;
        break;
    case BINARY_AND:
        op = IR_AND;
        break;
    case BINARY_OR:
        op = IR_OR;
        break;
    case BINARY_XOR:
        op = IR_XOR;
        break;
    case BINARY_AND_OP:
        abort();
        break;
    case BINARY_OR_OP:
        abort();
        break;
    }
    IrRegister * dest = emit_arith(irgen, op, left, right);

    if (post_op_negate)
    {
        return emit_arith(irgen, IR_NOT, dest, NULL);
    }
    return dest;
}

static IrRegister *walk_expr_unary(IrGenerator *irgen, ExprAstNode *node)
{
    if (node->unary.op == UNARY_DEREFERENCE)
    {
        return emit_load(irgen, node);
    }

    IrRegister *right = walk_expr(irgen, node->unary.right);

    switch (node->unary.op)
    {
    case UNARY_ADDRESS_OF:
        abort();
        break;
    case UNARY_PLUS:
        return right;
    case UNARY_MINUS:
        return emit_arith(irgen, IR_SUB, emit_loadi(irgen, 0), right);
    case UNARY_BITWISE_NOT:
        return emit_arith(irgen, IR_FLIP, right, NULL);
    case UNARY_LOGICAL_NOT:
        return emit_arith(irgen, IR_NOT, right, NULL);
    case UNARY_SIZEOF:
        abort();
        break;
    case UNARY_INC_OP: {
        int incr = node->unary.ptr_scale ? node->unary.ptr_scale : 1;
        IrRegister * dest = emit_arith(irgen, IR_ADD, right, emit_loadi(irgen, incr));
        emit_store(irgen, node->unary.right, dest);
        return dest;
    }
    case UNARY_DEC_OP: {
        int incr = node->unary.ptr_scale ? node->unary.ptr_scale : 1;
        IrRegister * dest = emit_arith(irgen, IR_SUB, right, emit_loadi(irgen, incr));
        emit_store(irgen, node->unary.right, dest);
        return dest;
    }
    }
    return NULL;
}

static IrRegister *walk_expr_primary(IrGenerator *irgen, ExprAstNode *node)
{
    if (node->primary.constant)
    {
        return emit_loadi(irgen, node->primary.constant->literal.const_value);
    }
    else if (node->primary.identifier)
    {
        Symbol *sym = node->primary.symbol;
        if (sym->ir.object)
        {
            // Object is not in a register.
            return emit_loadso(irgen, sym->ir.object->offset);
        }
        else if (sym->ir.regster)
        {
            // Object is in a register.
            IrRegister *value = sym->ir.regster;
            return emit_arith(irgen, IR_MOV, value, NULL);
        }
    }
    else if (node->primary.string_literal)
    {
        abort();
    }
    return NULL;
}

static IrRegister *walk_expr_postfix_call(IrGenerator *irgen, ExprAstNode *node)
{
    // What to do here?
    if (node->postfix.left->type == UNARY &&
        node->postfix.left->unary.op == UNARY_DEREFERENCE)
    {
        // This is a function pointer. Urgh, what a pain.
        // Think on this one alex.
        abort();
    }
    else
    {
        // This is just a straight up function call :-)
        int i = 0;
        for (ArgumentListItem *arg = node->postfix.args; arg != NULL; arg = arg->next)
        {
            IrRegister *arg_reg = walk_expr(irgen, arg->argument);
            IrRegister *param_reg = get_reg_reserved(irgen, i++);
            EMIT(irgen, IR_MOV, .dest = param_reg, .left = arg_reg);
        }
        EMIT(irgen, IR_CALL, .control.callee=node->postfix.left->primary.symbol->ir.function);

        // We need to copy out the return value.
        IrRegister * ret = get_reg_any(irgen);
        EMIT(irgen, IR_MOV, .dest=ret, .left=get_reg_reserved(irgen, 0));
        return ret;
    }
}

static IrRegister *walk_expr_postfix(IrGenerator *irgen, ExprAstNode *node)
{
    if (node->postfix.op == POSTFIX_CALL)
    {
        return walk_expr_postfix_call(irgen, node);
    }

    IrRegister *left = walk_expr(irgen, node->postfix.left);
    IrRegister *copy = get_reg_any(irgen);

    EMIT(irgen, IR_MOV, .dest = copy, .left = left);

    if (node->postfix.op == POSTFIX_INC_OP)
    {
        int increment = node->postfix.ptr_scale ? node->postfix.ptr_scale : 1;
        EMIT(irgen, IR_ADD, .dest = left, .left = left,
             .right = emit_loadi(irgen, increment));
    }
    else if (node->postfix.op == POSTFIX_DEC_OP)
    {
        int increment = node->postfix.ptr_scale ? node->postfix.ptr_scale : 1;
        EMIT(irgen, IR_SUB, .dest = left, .left = left,
             .right = emit_loadi(irgen, increment));
    }

    emit_store(irgen, node->postfix.left, left);

    return copy;
}

static IrRegister *walk_expr_cast(IrGenerator *irgen, ExprAstNode *node)
{
    IrRegister *right = walk_expr(irgen, node->cast.right);

    // Sign-extend.
    if (CTYPE_IS_SIGNED(node->cast.from) && CTYPE_IS_SIGNED(node->cast.to))
    {
        if (node->cast.from->basic.type_specifier & TYPE_CHAR)
        {
            EMIT(irgen, IR_SIGN_EXTEND_8, .dest = right, .left = right);
        }
        else if (node->cast.from->basic.type_specifier & TYPE_SHORT)
        {
            EMIT(irgen, IR_SIGN_EXTEND_16, .dest = right, .left = right);
        }
    }

    // Truncate.
    if (node->cast.to->basic.type_specifier & TYPE_CHAR)
    {
        EMIT(irgen, IR_AND, .dest = right, .left = emit_loadi(irgen, 0xFF),
             .right = right);
    }
    else if (node->cast.to->basic.type_specifier & TYPE_SHORT)
    {
        EMIT(irgen, IR_AND, .dest = right, .left = emit_loadi(irgen, 0xFFFF),
             .right = right);
    }

    return right;
}

static IrRegister *walk_expr_tertiary(IrGenerator *irgen, ExprAstNode *node)
{
    IrRegister *cond_reg = walk_expr(irgen, node->tertiary.condition_expr);
    IrRegister *result_reg = get_reg_any(irgen);

    IrBasicBlock *true_bb = new_bb(irgen, irgen->current_function);
    IrBasicBlock *false_bb = new_bb(irgen, irgen->current_function);
    IrBasicBlock *end_bb = new_bb(irgen, irgen->current_function);

    emit_jumpz(irgen, true_bb, false_bb, cond_reg);

    irgen->current_basic_block = true_bb;
    EMIT(irgen, IR_MOV, .dest = result_reg,
         .left = walk_expr(irgen, node->tertiary.expr_true));
    emit_jump(irgen, end_bb);

    irgen->current_basic_block = false_bb;
    EMIT(irgen, IR_MOV, .dest = result_reg,
         .left = walk_expr(irgen, node->tertiary.expr_false));
    emit_jump(irgen, end_bb);

    irgen->current_basic_block = end_bb;
    return result_reg;
}
static IrRegister *walk_expr_assign(IrGenerator *irgen, ExprAstNode *node)
{
    IrRegister *value = walk_expr(irgen, node->assign.right);
    emit_store(irgen, node->assign.left, value);
    return value;
}

static IrRegister *walk_expr(IrGenerator *irgen, ExprAstNode *node)
{
    switch (node->type)
    {
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
    return NULL;
}

static void walk_decl_function(IrGenerator *irgen, DeclAstNode *node)
{
    IrFunction *func = node->symbol->ir.function;

    func->registers.list = calloc(sizeof(IrRegister**), 32);
    func->registers.list_size = 32;

    // node->symbol->ir.function = func;
    irgen->current_function = func;

    irgen->current_basic_block = new_bb(irgen, func);

    int i = 0;
    for (ActualParameterListItem *arg = node->args; arg != NULL; arg = arg->next)
    {
        IrRegister *argument_body = get_reg_any(irgen);
        IrRegister *argument_proto = get_reg_reserved(irgen, i++);

        arg->sym->ir.regster = argument_body;
        EMIT(irgen, IR_MOV, .dest = argument_body, .left = argument_proto);
    }
    walk_stmt(irgen, node->body);

    EMIT(irgen, IR_RETURN);
}

static void walk_decl_object(IrGenerator *irgen, DeclAstNode *node)
{
    if (!irgen->current_function)
    {
        // Global
        abort();
    }
    else if (CTYPE_IS_SCALAR(node->type))
    {
        // Local - allocate in register.
        if (node->initializer)
        {
            IrRegister * dest = emit_arith(irgen, IR_MOV, walk_expr(irgen, node->initializer), NULL);
            node->symbol->ir.regster = dest;
        } else {
            node->symbol->ir.regster = emit_loadi(irgen, 0);
        }
    }
    else
    {
        // Local - allocate on the stack.
        IrObject *object = stack_allocate(irgen->current_function, node->symbol);
        node->symbol->ir.object = object;

        if (node->initializer)
        {
            abort();
        }
    }
}

static void walk_decl(IrGenerator *irgen, DeclAstNode *node)
{
    if(CTYPE_IS_FUNCTION(node->type))
    {
        // We are declaring a new function
        if(node->body) walk_decl_function(irgen, node);
    }
    else
    {
        // We are declaring an object.
        walk_decl_object(irgen, node);
    }

    if (node->next)
    {
        walk_decl(irgen, node->next);
    }
}

static void walk_stmt_while(IrGenerator *irgen, StmtAstNode *node)
{
    IrBasicBlock *loop_init_bb = new_bb(irgen, irgen->current_function);
    IrBasicBlock *while_bb = new_bb(irgen, irgen->current_function);
    IrBasicBlock *end_bb = new_bb(irgen, irgen->current_function);

    emit_jump(irgen, loop_init_bb);
    irgen->current_basic_block = loop_init_bb;

    IrRegister *condition_reg = walk_expr(irgen, node->while_loop.expr);
    emit_jumpz(irgen, while_bb, end_bb, condition_reg);

    irgen->current_basic_block = while_bb;
    walk_stmt(irgen, node->while_loop.block);
    emit_jump(irgen, loop_init_bb);

    irgen->current_basic_block = end_bb;
}

static void walk_stmt_return(IrGenerator *irgen, StmtAstNode *node)
{
    if (node->return_jump.value)
    {
        IrRegister *val = walk_expr(irgen, node->return_jump.value);
        EMIT(irgen, IR_MOV, .dest = get_reg_reserved(irgen, 0), .left = val);
    }
    EMIT(irgen, IR_RETURN);
}

static void walk_stmt_if(IrGenerator *irgen, StmtAstNode *node)
{
    IrRegister *expr_reg = walk_expr(irgen, node->if_statement.expr);

    IrBasicBlock *true_bb = new_bb(irgen, irgen->current_function);
    IrBasicBlock *end_bb = new_bb(irgen, irgen->current_function);

    if (node->if_statement.else_arm)
    {
        IrBasicBlock *else_bb = new_bb(irgen, irgen->current_function);
        emit_jumpz(irgen, true_bb, else_bb, expr_reg);

        irgen->current_basic_block = end_bb;
        walk_stmt(irgen, node->if_statement.else_arm);
    }
    else
    {
        emit_jumpz(irgen, true_bb, end_bb, expr_reg);
    }

    irgen->current_basic_block = true_bb;
    walk_stmt(irgen, node->if_statement.if_arm);

    irgen->current_basic_block = end_bb;
}

static void walk_stmt(IrGenerator *irgen, StmtAstNode *node)
{
    if (node == NULL)
        return;

    switch (node->type)
    {
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
    case IF_STATEMENT:
        walk_stmt_if(irgen, node);
        break;
    }

    walk_stmt(irgen, node->next);
}

IrFunction *Ir_generate(DeclAstNode *ast_root, SymbolTable * tab)
{
    IrGenerator irgen = {NULL, NULL};
    IrFunction * head = NULL;

    for(DeclAstNode * f = ast_root;f;f=f->next)
    {
        if(!CTYPE_IS_FUNCTION(f->type) || f->symbol->ir.function) continue;

        IrFunction * function = new_function(&irgen, f->identifier->lexeme);
        f->symbol->ir.function = function;
        function->next = head;
        head = function;
    }   

    walk_decl(&irgen, ast_root);

    return head;
}