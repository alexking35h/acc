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
    IrRegister test_reg = {
        .index = 0, .type = REG_ANY, .liveness.start = -1, .liveness.finish = 0};
    IrInstruction nop = {IR_NOP, .pos = 0};
    IrInstruction assign = {IR_LOADI, .dest = &test_reg, .value = 1, .prev = &nop,
                            .pos = 1};
    IrInstruction add = {
        IR_ADD,          .dest = &test_reg, .left = &test_reg, .right = &test_reg,
        .prev = &assign, .pos = 2};
    IrInstruction ret = {IR_RETURN, .prev = &add, .pos = 3};

    IrBasicBlock bb = {.head = &nop, .tail = &ret};
    IrFunction main = {.name = "main", .head = &bb, .tail = &bb, .register_count = 1};
    IrProgram program = {.head = &main, .tail = &main};

    Liveness_analysis(&program);

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
    IrRegister test_reg = {.index = 0, .type = REG_ANY, .liveness.start = -1, .liveness.finish = 0};

    IrInstruction add = {IR_ADD, .dest=&test_reg, .left=&test_reg, .right=&test_reg, .pos=3};
    IrInstruction ret = {IR_RETURN, .prev=&add, .pos=4};
    IrBasicBlock bb2 = {.head=&add, .tail=&ret};

    IrInstruction jump_b2 = {IR_JUMP, .jump=&bb2, .pos=5};
    IrBasicBlock bb3 = {.head=&jump_b2, .tail=&jump_b2};

    IrInstruction nop = {IR_NOP, .pos=0};
    IrInstruction assign = {IR_LOADI, .dest=&test_reg, .value=1, .pos=1, .prev=&nop};
    IrInstruction jump_b3 = {IR_JUMP, .jump=&bb3, .pos=2, .prev=&assign};
    IrBasicBlock bb1 = {.head=&assign, .tail=&jump_b3};

    bb1.next = &bb2;
    bb2.next = &bb3;

    bb2.entry[0] = &bb3;
    bb3.entry[0] = &bb1;

    IrFunction main = {.name="main", .head=&bb1, .tail=&bb3, .register_count=1, .registers=&test_reg};
    IrProgram program = {.head=&main, .tail=&main};

    Liveness_analysis(&program);

    assert_true(test_reg.liveness.start == 1);
    assert_true(test_reg.liveness.finish == 5);
}

int main(void)
{
    const struct CMUnitTest tests[] = {cmocka_unit_test(liveness_basic_block),
                                       cmocka_unit_test(liveness_loop)};

    return cmocka_run_group_tests(tests, NULL, NULL);
}