#if 0
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "ir.h"

typedef struct RegisterStack
{
    int reg_number;
    int expire_at;

    struct RegisterStack *next;
} RegisterStack;

typedef struct IrRegisterSet
{
    IrRegister *left;
    IrRegister *right;
    IrRegister *dest;
    IrRegister *temp;
    IrRegister *sp;
} IrRegisterSet;

static int stack_allocate(IrFunction *function, int size, int align)
{
    int offset = function->stack_size;
    function->stack_size += (size + 3) & ~3;
    return offset;
}

/*
 * Allocate a real register.
 *
 * Removes a register from the 'free' stack, and insert it onto the 'used' stack.
 */
static _Bool reg_alloc(RegisterStack **free, RegisterStack **used, int *reg_number,
                       int expire_at)
{
    if (*free == NULL)
    {
        return false;
    }

    RegisterStack *reg = *free;
    *free = (*free)->next;

    reg->expire_at = expire_at;
    reg->next = *used;

    *used = reg;

    *reg_number = reg->reg_number;
    return true;
}

/*
 * Remove registers from the 'used' stack after they have expired.
 *
 * Registers are 'expired' once their live range is no longer reachable.
 */
static void reg_expire(RegisterStack **free, RegisterStack **used, int line)
{
    for (RegisterStack **u = used; *u != NULL;)
    {
        if ((*u)->expire_at < line)
        {
            // This registers live range has expired. Remove it from the 'used' stack,
            // and push it onto the free stack.
            *used = (*u)->next;

            (*u)->next = (*free);
            (*free) = (*u);
        }

        u = &((*u)->next);
    }
}

/*
 * Return a list of registers sorted by increasing start live positions.
 */
IrRegister **register_sort(IrRegister *reg_head)
{
    IrRegister **sorted = calloc(reg_head->index + 1, sizeof(IrRegister *));
    int tail = 0;

    while (reg_head)
    {
        int i = tail;
        for (; i > 0; i--)
        {
            if (sorted[i - 1]->liveness.start > reg_head->liveness.start)
            {
                sorted[i] = sorted[i - 1];
            }
            else
            {
                break;
            }
        }
        sorted[i] = reg_head;
        tail++;

        reg_head = reg_head->next;
    }

    return sorted;
}

static void allocate_registers(IrFunction *function, int *register_numbers)
{
    IrRegister **registers = register_sort(function->registers);

    RegisterStack *used = NULL, *free;
    for (int *n = register_numbers; *n != -1; n++)
    {
        RegisterStack *reg = calloc(1, sizeof(RegisterStack));

        reg->reg_number = *(register_numbers++);
        reg->next = free;
        free = reg;
    }

    for (int i = 0; i < function->register_count; i++)
    {
        reg_expire(&free, &used, registers[i]->liveness.start);

        if (!reg_alloc(&free, &used, &registers[i]->real, registers[i]->liveness.finish))
        {
            registers[i]->spill = stack_allocate(function, 4, 4);
            printf("spill r%d to %d\n", registers[i]->index, registers[i]->spill);
        }
        else
        {
            printf("allocate r%d to r%d\n", registers[i]->index, registers[i]->real);
        }
    }
}

static void emit_store(IrInstruction *after, IrRegister *source, IrRegisterSet *reg_set,
                       int stack_loc)
{
    // temp = stack_loc
    // temp = temp + sp
    // *temp = source
    IrInstruction *loadi = calloc(1, sizeof(IrInstruction));
    loadi->op = IR_LOADI;
    loadi->value = stack_loc;
    loadi->dest = reg_set->temp;

    IrInstruction *add = calloc(1, sizeof(IrInstruction));
    add->op = IR_ADD;
    add->left = reg_set->temp;
    add->right = reg_set->sp;
    add->dest = reg_set->temp;

    IrInstruction *store = calloc(1, sizeof(IrInstruction));
    store->op = IR_STORE32;
    store->left = reg_set->temp;
    store->right = source;

    // links...
    loadi->next = add;
    loadi->prev = after;

    add->next = store;
    add->prev = loadi;

    store->next = after->next;
    store->prev = add;

    after->prev = store;
}

static void emit_load(IrInstruction *before, IrRegister *dest, IrRegisterSet *reg_set,
                      int stack_loc)
{
    // temp = stack_loc
    // temp = temp + sp
    // dest = *temp

    IrInstruction *loadi = calloc(1, sizeof(IrInstruction));
    loadi->op = IR_LOADI;
    loadi->value = stack_loc;
    loadi->dest = reg_set->temp;

    IrInstruction *add = calloc(1, sizeof(IrInstruction));
    add->op = IR_ADD;
    add->left = reg_set->temp;
    add->right = reg_set->sp;
    add->dest = reg_set->temp;

    IrInstruction *load = calloc(1, sizeof(IrInstruction));
    load->op = IR_LOAD32;
    load->dest = dest;
    load->left = reg_set->temp;

    // Links...
    before->prev->next = loadi;

    loadi->next = add;
    loadi->prev = before->prev;

    add->next = load;
    add->prev = loadi;

    load->next = before;
    load->prev = add;

    before->prev = load;
}

static void fixup_init(IrRegisterSet *register_set, int *temp_regs)
{
    register_set->dest = calloc(1, sizeof(IrRegister));
    register_set->dest->real = temp_regs[0];
    register_set->dest->type = REG_ANY;

    register_set->left = calloc(1, sizeof(IrRegister));
    register_set->left->real = temp_regs[1];
    register_set->left->type = REG_ANY;

    register_set->right = calloc(1, sizeof(IrRegister));
    register_set->right->real = temp_regs[2];
    register_set->right->type = REG_ANY;

    register_set->temp = calloc(1, sizeof(IrRegister));
    register_set->temp->real = temp_regs[3];
    register_set->temp->type = REG_ANY;

    register_set->sp = calloc(1, sizeof(IrRegister));
    register_set->sp->type = REG_STACK;
}

static void fixup_bb(IrBasicBlock *bb, IrRegisterSet *reg_set)
{
    // Prev:
    // dest = left OP right

    // After:
    // tmp = i + sp
    // left = *tmp
    // tmp = j + sp
    // right = *tmp
    // dest = left OP right
    // tmp = m + sp
    // *tmp = dest

    for (IrInstruction *instr = bb->head; instr;)
    {
        IrInstruction *next = instr->next;
        if (instr->left && instr->left->type == REG_ANY && instr->left->spill)
        {
            emit_load(instr, reg_set->left, reg_set, instr->left->spill);
            instr->left = reg_set->left;
        }
        if (instr->right && instr->right->type == REG_ANY && instr->right->spill)
        {
            emit_load(instr, reg_set->right, reg_set, instr->right->spill);
            instr->right = reg_set->right;
        }
        // if(instr->dest && instr->dest->type == REG_ANY && instr->dest->spill)
        // {
        //     emit_store(instr, reg_set->dest, reg_set, instr->dest->spill);
        //     instr->dest = reg_set->dest;
        // }
        instr = next;
    }
}

void regalloc(IrProgram *prog)
{
    int temp_regs[] = {3, 4, 5, 6};
    int register_numbers[] = {7, 8, 9, -1};

    IrRegisterSet reg_temp_set;
    fixup_init(&reg_temp_set, temp_regs);

    for (IrFunction *function = prog->head; function; function = function->next)
    {
        stack_allocate(function, 1, 1);
        allocate_registers(function, register_numbers);

        for (IrBasicBlock *bb = function->head; bb; bb = bb->next)
        {
            fixup_bb(bb, &reg_temp_set);
        }
    }
}
#endif 