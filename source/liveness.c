#include <stdint.h>
#include <stdlib.h>

#include "ir.h"
#include "liveness.h"

#define MAX(a, b) ((a) < (b) ? (b) : (a))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

static uint8_t *register_set_init(int sz)
{
    return calloc(sz / 8 + 1, sizeof(uint8_t));
}

// Mark a register 'index' within a set
static void register_set_mark(uint8_t *set, int index)
{
    set[index / 8] |= 1 << index % 8;
}

// Remove register 'index' from a set
static void register_set_unmark(uint8_t *set, int index)
{
    set[index / 8] &= ~(1 << index % 8);
}

// Test if register 'index' is in a set
static _Bool register_set_test(uint8_t *set, int index)
{
    return set[index / 8] & 1 << index % 8;
}

// Union operation: set_A = set_A U set_B
// Return False if sets already identical.
static _Bool register_set_union(uint8_t *set_A, uint8_t *set_B, int sz)
{
    uint8_t changed = 0;
    for(int i = 0;i < sz / 8 + 1;i++)
    {
        // Alex - should this be == ? What if a node has two parents?
        // The parents can have different ENTRY sets...
        uint8_t t = set_A[i] | set_B[i];
        if(t != set_A[i]) 
        {
            changed++;
            set_A[i] = t;
        }
    }
    return changed != 0;
}

// 'Use' a register. This adds a register to the 'entry' set in a BB.
// This also updates the liveness start position.
static void reg_define(IrBasicBlock *bb, IrRegister *reg, int position)
{
    if (!reg)
        return;

    register_set_unmark(bb->live.entry, reg->index);
    reg->liveness.start = MIN(reg->liveness.start, position);
    reg->liveness.finish = MAX(reg->liveness.finish, position);
}

// 'Define' a register. This removes a register from the 'entry' set in a BB.
// This also updates the liveness finish position.
static void reg_use(IrBasicBlock *bb, IrRegister *reg, int position)
{
    if (!reg)
        return;

    register_set_mark(bb->live.entry, reg->index);
    reg->liveness.start = MIN(reg->liveness.start, position);
    reg->liveness.finish = MAX(reg->liveness.finish, position);
}

static int basic_block(IrBasicBlock *bb, int sz)
{
    int changed = 0;
    register_set_union(bb->live.entry, bb->live.exit, sz);

    for (IrInstruction *instr = bb->tail;; instr = instr->prev)
    {
        reg_define(bb, instr->dest, instr->live_position);
        reg_use(bb, instr->left, instr->live_position);
        reg_use(bb, instr->right, instr->live_position);

        if (instr == bb->head)
            break;
    }

    // For each precursor basic block, set the EXIT set to include
    // all registers in this basic block's ENTRY registers. 
    if (bb->cfg_entry[0])
    {
        changed += register_set_union(bb->cfg_entry[0]->live.exit, bb->live.entry, sz) ? 1 : 0;
    }
    if (bb->cfg_entry[1])
    {
        changed += register_set_union(bb->cfg_entry[1]->live.exit, bb->live.entry, sz) ? 1 : 0;
    }
    return changed;
}

static void function_begin(IrFunction *function)
{
    int instr_index = 0;
    for (IrBasicBlock *bb = function->head;bb;bb = bb->next)
    {
        bb->live.entry = register_set_init(function->registers.count);
        bb->live.exit = register_set_init(function->registers.count);

        for(IrInstruction * instr = bb->head;instr;instr = instr->next)
        {
            instr->live_position = instr_index++;
        }
    }
}

static void function_end(IrFunction *function)
{
    for (IrBasicBlock *bb = function->head; bb; bb = bb->next)
    {
        for(int i = 0;i < function->registers.count; i++)
        {
            int reg_idx = function->registers.list[i]->index;

            // See if we have a register that is in the entry and exit
            // sets for this basic block.
            if (!register_set_test(bb->live.entry, reg_idx))
                continue;
            if (!register_set_test(bb->live.exit, reg_idx))
                continue;

            int bb_start = bb->head->live_position;
            int bb_finish = bb->tail->live_position;

            function->registers.list[reg_idx]->liveness.start = 
                MIN(function->registers.list[i]->liveness.start, bb_start);
            function->registers.list[reg_idx]->liveness.finish = 
                MAX(function->registers.list[i]->liveness.finish, bb_finish);
        }
    }
}

// Live analysis overview:
// While basic block entry and exit sets are changing
//     For every basic block: b
//         For every instruction in reverse order: instr
//             For instr register left, right, dest: reg
//                 Set reg.start = min(reg.start, instr.idx)
//                 Set reg.finish = max(reg.finish, instr.idx)
//             Remove instr.dest from b.entry
//             Add instr.left and instr.right to b.entry
//
//         For all preceeding BBs: pb
//             Set pb.exit = union(pb.exit, pb.entry)
//
// For every basic block: b
//     For every register: reg
//         if reg in b.entry and reg in p.exit:
//             Set reg.start = min(reg.start, b.start)
//             Set reg.finish = max(reg.finish, b.finish)
static void function(IrFunction *function)
{
    function_begin(function);

    int changed = 1;
    while (changed != 0)
    {
        changed = 0;
        for (IrBasicBlock *bb = function->head; bb; bb = bb->next)
        {
            changed += basic_block(bb, function->registers.count);
        }
    }

    function_end(function);
}

void Liveness_analysis(IrFunction *program)
{
    for(IrFunction * func = program;func;func=func->next)
    {
        function(func);
    }
}