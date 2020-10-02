#include <stdint.h>
#include <stdlib.h>

#include "ir.h"
#include "liveness.h"

#define MAX(a, b) ((a) < (b) ? (b) : (a))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

#if 0

static uint32_t *register_set_init(int sz)
{
    return calloc(sz / 32 + 1, sizeof(uint32_t));
}

static void register_set_mark(uint32_t *set, int index)
{
    set[index / 32] |= 1 << index % 32;
}

static void register_set_unmark(uint32_t *set, int index)
{
    set[index / 32] &= ~(1 << index % 32);
}

static _Bool register_set_test(uint32_t *set, int index)
{
    return set[index / 32] & 1 << index % 32;
}

static _Bool register_set_union(uint32_t *set_A, uint32_t *set_B, int sz)
{
    uint32_t j = 0;
    for (int i = 0; i * 4 < 4; i++)
    {
        j += (set_A[i] != set_B[i] ? 1 : 0);
        set_A[i] |= set_B[i];
    }
    return j == 0 ? false : true;
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

static void reg_define(IrBasicBlock *bb, IrRegister *reg, int position)
{
    if (!reg)
        return;

    register_set_unmark(bb->entry_set, reg->index);
    reg->liveness.start = MIN(reg->liveness.start, position);
}

static void reg_use(IrBasicBlock *bb, IrRegister *reg, int position)
{
    if (!reg)
        return;

    register_set_mark(bb->entry_set, reg->index);
    reg->liveness.finish = MAX(reg->liveness.finish, position);
}

static int basic_block(IrBasicBlock *bb, int sz)
{
    int changed = 0;
    register_set_union(bb->entry_set, bb->exit_set, sz) ? 1 : 0;

    for (IrInstruction *instr = bb->tail;; instr = instr->prev)
    {
        reg_define(bb, instr->dest, instr->pos);
        reg_use(bb, instr->left, instr->pos);
        reg_use(bb, instr->right, instr->pos);

        if (instr == bb->head)
            break;
    }

    // For each precursor basic block, set the EXIT set to include
    // all registers in this basic block's ENTRY registers.
    if (bb->entry[0])
    {
        changed += register_set_union(bb->entry[0]->exit_set, bb->entry_set, sz) ? 1 : 0;
    }
    else if (bb->entry[1])
    {
        changed += register_set_union(bb->entry[1]->exit_set, bb->entry_set, sz) ? 1 : 0;
    }

    return changed;
}

static void function_begin(IrFunction *function)
{
    for (IrBasicBlock *bb = function->head;; bb = bb->next)
    {
        bb->entry_set = register_set_init(function->register_count);
        bb->exit_set = register_set_init(function->register_count);

        if (bb == function->tail)
            break;
    }
}

static void function_end(IrFunction *function)
{
    for (int i = 0; i < function->register_count; i++)
    {
        for (IrBasicBlock *bb = function->head; bb; bb = bb->next)
        {
            // See if we have a register that is in the entry and exit
            // sets for this basic block.
            if (!register_set_test(bb->entry_set, i))
                continue;
            if (!register_set_test(bb->exit_set, i))
                continue;

            int bb_start = bb->head->pos;
            int bb_finish = bb->tail->pos;

            function->registers[i].liveness.start =
                MIN(function->registers[i].liveness.start, bb_start);
            function->registers[i].liveness.finish =
                MAX(function->registers[i].liveness.finish, bb_finish);
        }
    }
}

static void function(IrFunction *function)
{
    function_begin(function);

    int changed = 1;
    while (changed != 0)
    {
        changed = 0;
        for (IrBasicBlock *bb = function->head; bb; bb = bb->next)
        {
            changed += basic_block(bb, function->register_count);
        }
    }

    function_end(function);
}

void Liveness_analysis(IrProgram *program)
{
    for (IrFunction *func = program->head; func; func = func->next)
    {
        function(func);
    }
}

#endif