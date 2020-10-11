#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

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
            stack_push(free, reg->real.index);
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


static void regalloc_alloc(IrFunction * function, int registers[])
{
    FreeStack free = {
        .head = 0,
        .stack = calloc(5, sizeof(int))
    };
    for(int * reg_no = registers;*reg_no != -1;reg_no++)
    {
        stack_push(&free, *reg_no);
    }

    ActiveSet active = {
        .count = 0,
        .max = 5,
        .set = calloc(5, sizeof(IrRegister **))
    };

    registers_sort(function->registers.list, function->registers.count);

    for(int i = 0;i < function->registers.count;i++)
    {
        IrRegister * reg = function->registers.list[i];
        regalloc_expire_active(&active, &free, reg->liveness.start);

        // Try to allocate a free register.
        if(stack_pop(&free, &reg->real.index) == true)
        {
            active_add(&active, reg);
            continue;
        }

        // We've run out of free registers. Find the next available register.
        IrRegister * replace = active_get(&active);

        if(replace && replace->liveness.finish < reg->liveness.finish)
        {
            reg->real.index = replace->real.index;
            replace->real.spill = regalloc_spill(function);
            active_remove(&active, replace);
            active_add(&active, reg);
        } else {
            reg->real.spill = regalloc_spill(function);
        }
    }
}

static void regalloc_fixup(IrFunction * function)
{
    // TODO.
}

void regalloc(IrFunction * function, int reserved_registers[], int free_registers[])
{
    for(;function;function=function->next)
    {
        regalloc_alloc(function, free_registers);
        regalloc_fixup(function);
    }
}