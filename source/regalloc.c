#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "regalloc.h"
#include "ir.h"

// Linear Scan Register allocation
//
// For each live interval: i
//   Expire_old_intervals(i)
//   if len(free_registers) > 0:
//     i.register = free_registers.pop()
//     active += i
//   else:
//     Allocate_register(i)
//
// Allocate_register(live interval: i)
//   j = latest endpoint register in active
//   if i < j:
//     i.stack = stack_allocate()
//   else:
//     j.stack = stack_allocate()
//     i = j

typedef struct FreeStack
{
    int head;
    int *stack;
} FreeStack;

typedef struct ActiveSet
{
    int count;
    int max;
    IrRegister ** set;
} ActiveSet;

static void stack_push(FreeStack * stack, int index)
{
    stack->stack[stack->head++] = index;
}

static _Bool stack_pop(FreeStack * stack, int * index)
{
    if(stack->head == 0) return false;
    *index = stack->stack[--stack->head];
    return true;
}

static void active_add(ActiveSet * active, IrRegister * reg)
{
    assert(active->count < active->max);

    for(int i = 0;;i++)
    {
        if(active->set[i] == NULL)
        {
            active->set[i] = reg;
            active->count++;
            return;
        }
    }
}

static void active_remove(ActiveSet * active, IrRegister * reg)
{
    assert(active->count > 0);

    for(int i = 0;i < active->max;i++)
    {
        if(active->set[i] == reg)
        {
            active->set[i] = NULL;
            active->count--;
            return;
        }
    }
    assert(false && "Register not in active set");
}

static IrRegister * active_get(ActiveSet * active)
{
    int soonest = -1;
    IrRegister * reg = NULL;

    for(int i = 0;i < active->max;i++)
    {
        if(active->set[i] && active->set[i]->liveness.finish < soonest)
        {
            soonest = active->set[i]->liveness.finish;
            reg = active->set[i];
        }
    }
    return reg;
}

// Sort the list of registers within an IrFunction
// by increasing live start index.
static void registers_sort(IrRegister ** list, int count)
{
    for(int i = 1;i < count;i++)
    {
        IrRegister * focus = list[i];

        for(IrRegister ** ptr = &list[i-1];;ptr--)
        {
            if((*ptr)->liveness.start > focus->liveness.start)
            {
                *(ptr + 1) = *ptr;
                *ptr = focus;
            }

            if(ptr == list) break;
        }
    }
}

static void regalloc_expire_active(ActiveSet * active, FreeStack * free, int index)
{
    for(int i = 0;i < active->max;i++)
    {
        if(active->set[i] == NULL) continue;

        IrRegister * reg = active->set[i];

        if(reg->liveness.finish < index)
        {
            active_remove(active, reg);
            stack_push(free, reg->index);
        }
    }
}

static int regalloc_spill(IrFunction * function)
{
    // TODO.
    int spill = function->stack_size;
    function->stack_size += 4;
    return spill;
}


static void regalloc_alloc(IrFunction * function, int * free_regs)
{
    int free_registers_count = 0;
    for(;free_regs[free_registers_count] != -1;free_registers_count++);

    FreeStack free = {
        .head = 0,
        .stack = calloc(free_registers_count, sizeof(int))
    };
    for(int i = 0;i < free_registers_count;i++)
    {
        stack_push(&free, free_regs[i]);
    }

    ActiveSet active = {
        .count = 0,
        .max = free_registers_count,
        .set = calloc(free_registers_count, sizeof(IrRegister **))
    };

    registers_sort(function->registers.list, function->registers.count);

    for(int i = 0;i < function->registers.count;i++)
    {
        IrRegister * reg = function->registers.list[i];
        regalloc_expire_active(&active, &free, reg->liveness.start);

        // Try to allocate a free register.
        if(stack_pop(&free, &reg->index) == true)
        {
            active_add(&active, reg);
            continue;
        }

        // We've run out of free registers. Find the next available register.
        IrRegister * replace = active_get(&active);

        if(replace && replace->liveness.finish < reg->liveness.finish)
        {
            reg->index = replace->index;
            
            replace->type = REG_SPILL;
            replace->spill = regalloc_spill(function);

            active_remove(&active, replace);
            active_add(&active, reg);
        } else {
            reg->type = REG_SPILL;
            reg->spill = regalloc_spill(function);
        }
    }
}

/*
 * Emit spill code from spill_regs[1] -> stack@spill_loc
 */
static void emit_spill_store(IrInstruction * after, int spill_loc, IrRegister ** spill_regs)
{
    IrInstruction * loadso = calloc(1, sizeof(IrInstruction));
    loadso->op = IR_LOADSO;
    loadso->value = spill_loc;
    loadso->dest = spill_regs[0];
    Ir_emit_instr_after(after, loadso);

    IrInstruction * store32 = calloc(1, sizeof(IrInstruction));
    store32->op = IR_STORE32;
    store32->left = spill_regs[0];
    store32->right = spill_regs[1];
    Ir_emit_instr_after(loadso, store32);
}

/*
 * Emit spill code code from stack@spill_loc -> dest_reg
 */
static void emit_spill_load(IrInstruction * before, int spill_loc, IrRegister * dest, IrRegister ** spill_regs)
{
    IrInstruction * loadso = calloc(1, sizeof(IrInstruction));
    loadso->op = IR_LOADSO;
    loadso->value = spill_loc;
    loadso->dest = spill_regs[0];
    Ir_emit_instr_before(before, loadso);

    IrInstruction * load32 = calloc(1, sizeof(IrInstruction));
    load32->op = IR_LOAD32;
    load32->dest = dest;
    load32->left = spill_regs[0];
    Ir_emit_instr_before(before, load32);
}

static void regalloc_fixup(IrFunction * function, int * fixup_regs)
{
    // Create the spill register set.
    IrRegister * spill_regs[REGS_SPILL];
    for(int i = 0;i < REGS_SPILL;i++)
    {
        spill_regs[i] = calloc(1, sizeof(IrRegister));
        spill_regs[i]->type = REG_ANY;
        spill_regs[i]->index = fixup_regs[i];
    }
    IrRegister * spill_src = spill_regs[1];
    IrRegister * spill_dest_left = spill_regs[2];
    IrRegister * spill_dest_right = spill_regs[3];

    for(IrBasicBlock * bb = function->head;bb != NULL;bb = bb->next)
    {
        for(IrInstruction * instr = bb->head;instr != NULL;)
        {
            IrInstruction * next = instr->next;

            if(instr->dest && instr->dest->type == REG_SPILL)
            {
                emit_spill_store(instr, instr->dest->spill, spill_regs);
                instr->dest = spill_src;
            }

            if(instr->left && instr->left->type == REG_SPILL)
            {
                emit_spill_load(instr, instr->left->spill, spill_dest_left, spill_regs);
                instr->left = spill_dest_left;
            }
            if(instr->right && instr->right->type == REG_SPILL)
            {
                emit_spill_load(instr, instr->right->spill, spill_dest_right, spill_regs);
                instr->right = spill_dest_right;
            }

            instr = next;
        }
    }
}

void regalloc(IrFunction * function, int * free_registers)
{
    for(int i = 0;i < REGS_SPILL;i++) assert(free_registers[i] != -1);

    int * spill_regs = free_registers;
    int * free_regs = free_registers + REGS_SPILL;

    for(;function;function=function->next)
    {
        regalloc_alloc(function, free_regs);
        regalloc_fixup(function, spill_regs);
    }
}