#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>

#include <cmocka.h>

#include "ir.h"
#include "liveness.h"

static void liveness_basic_block(void **state)
{
    // Test Liveness analysis on a single basic block:
    //  function main:
    //    BB 0:
    //      0 nop
    //      1 r0 = 1
    //      2 r0 = r0 + r0
    //      3 return

    // clang-format off
    IrRegister test_reg = {
        .virtual_index = 0,
        .type = REG_ANY,
        .liveness = {
            .start = -1,
            .finish = 0
        }
    };
    IrRegister * reg_list[] = {&test_reg};

    IrInstruction nop = {
        .op = IR_NOP
    };
    IrInstruction assign = {
        .op = IR_LOADI,
        .dest = &test_reg,
        .value = 1, 
        .prev = &nop
    };
    IrInstruction add = {
        .op = IR_ADD,
        .dest = &test_reg,
        .left = &test_reg,
        .right = &test_reg,
        .prev = &assign
    };
    IrInstruction ret = {
        .op = IR_RETURN,
        .prev = &add
    };
    nop.next = &assign;
    assign.next = &add;
    add.next = &ret;

    IrBasicBlock bb = {
        .head = &nop,
        .tail = &ret
    };
    IrFunction main = {
        .name = "main",
        .head = &bb,
        .tail = &bb,
        .registers = {
            .count = 1,
            .list = reg_list
        }
    };
    //clang-format on

    Liveness_analysis(&main);

    // Check that the live range for r0 is [1,2].
    assert_true(test_reg.liveness.start == 1);
    assert_true(test_reg.liveness.finish == 2);
}


static void liveness_loop(void **state)
{
    // Test Liveness analysis on a more complex CFG:
    //    BB 1:
    //      0 nop
    //      1 r0 = 1
    //      2 jump B.3
    //    BB 2:
    //      3 add r0 = r0 + r0
    //      4 return
    //    BB 3:
    //      5 jump B.2
    //      6 sub ...

    // clang-format off
    IrRegister test_reg = {
        .virtual_index = 0,
        .type = REG_ANY,
        .liveness = {
            .start = -1,
            .finish = 0
        }
    };
    IrRegister * reg_list[] = {&test_reg};

    // BB 1:
    IrInstruction nop = {
        .op = IR_NOP
    };
    IrInstruction assign = {
        .op = IR_LOADI,
        .dest = &test_reg,
        .value = 1,
        .prev = &nop
    };
    IrInstruction jump_bb1 = {
        .op = IR_JUMP,
        .prev = &assign
    };
    nop.next = &assign;
    assign.next = &jump_bb1;

    // BB 2:
    IrInstruction add = {
        .op = IR_ADD,
        .dest = &test_reg,
        .left = &test_reg,
        .right = &test_reg
    };
    IrInstruction ret = {
        .op = IR_RETURN,
        .prev = &add
    };
    add.next = &ret;

    // BB 3:
    IrInstruction jump_bb3 = {
        .op = IR_JUMP,
    };
    IrInstruction sub = {
        .op = IR_SUB,
        .prev = &jump_bb3
    };
    jump_bb3.next = &sub;

    IrBasicBlock bb1 = {
        .head = &nop,
        .tail = &jump_bb1,
    };
    IrBasicBlock bb3 = {
        .head = &jump_bb3,
        .tail = &sub,
        .cfg_entry = {&bb1}
    };
    IrBasicBlock bb2 = {
        .head = &add,
        .tail = &ret,
        .cfg_entry = {&bb3}
    };

    bb1.next = &bb2;
    bb2.next = &bb3;

    IrFunction main = {
        .name = "main",
        .head = &bb1,
        .tail = &bb3,
        .registers = {
            .count = 1,
            .list = reg_list
        }
    };
    // clang-format on

    Liveness_analysis(&main);

    assert_true(test_reg.liveness.start == 1);
    assert_true(test_reg.liveness.finish == 6);
}

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(liveness_basic_block),
        cmocka_unit_test(liveness_loop)
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}