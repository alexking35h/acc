#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

#include <cmocka.h>

#include "ir.h"
#include "liveness.h"

void regalloc(IrFunction *, int[], int[]);

static void regalloc_no_spill()
{
    // Test the Linear scan register allocation, with no regist;
    IrRegister regA = {
        .type = REG_ANY,
        .liveness = {0, 8}
    };
    IrRegister regB = {
        .type = REG_ANY,
        .liveness = {1, 4}
    };
    IrRegister regC = {
        .type = REG_ANY,
        .liveness = {5, 10}
    };

    // Registers are out-of-order. regalloc() should sort these.
    IrRegister * reg_list[] = {&regC, &regB, &regA};

    IrFunction function = {
        .registers = {
            .count = 3,
            .list = reg_list
        }
    };

    regalloc(
        &function,
        (int[]){1,2,3,4,-1},
        (int[]){5,6,-1}
    );
    assert_true(reg_list[2] == &regC);
    assert_true(regC.real.spill == 0);

    // Check the register allocations.
    assert_true(regA.real.index == 6);
    assert_true(regB.real.index == 5);
    assert_true(regC.real.index == 5);
}

static void regalloc_no_fixup()
{
    // Test the Linear Scan register allocation only.
    // This test does not verify the 'fixup' stage - where spill
    // code is inserted into the IR code.
    //
    // This test should include register spills where:
    //  - a register needs to be removed from the active set
    //  - a register should never be allocated to begin with.

    // Linear scan:
    // 1. Allocate reg A to register.
    // 2. Allocate register B to register.
    // 3. Spill register B, allocate register C (regB.finish < regC.finish)
    // 4. Spill register D (regD.finish < regC.finish)
    IrRegister regA = {
        .type = REG_ANY,
        .liveness = {
            .start = 0,
            .finish = 10
        },
    };
    IrRegister regB = {
        .type = REG_ANY,
        .liveness = {
            .start = 1,
            .finish = 5
        }
    };
    IrRegister regC = {
        .type = REG_ANY,
        .liveness = {
            .start = 2,
            .finish = 7
        }
    };
    IrRegister regD = {
        .type = REG_ANY,
        .liveness = {
            .start = 3,
            .finish = 5
        }
    };

    IrRegister * reg_list[] = {&regD, &regC, &regB, &regA};

    // Registers are out-of-order, regalloc() should sort thes;
    IrFunction function = {
        .registers = {
            .count = 4,
            .list = reg_list
        }
    };

    regalloc(
        &function,
        (int[]){1,2,3,4,-1},
        (int[]){5,6,-1}
    );
    assert_true(regC.real.index == 5);
    assert_true(regC.real.spill == 0);
}

static void compare_reg(IrRegister * regA, IrRegister * regB)
{
    if(regA == NULL)
    {
        assert_true(regB == NULL);
    }
    else {
        assert_true(regB);
        assert_true(regA->real.index == regB->real.index);
    }
}

static void regalloc_fixup_store()
{
    IrRegister regA = {
        .type = REG_ANY,
        .virtual_index = 1,
        .liveness = {
            .start = 0,
            .finish = 1
        }
    };
    IrInstruction nop = {
        .op = IR_NOP
    };
    IrInstruction loadi = {
        .op = IR_LOADI,
        .value = 99,
        .dest = &regA
    };
    nop.next = &loadi;

    IrBasicBlock bb = {
        .head = &nop,
        .tail = &loadi
    };
    IrFunction func = {
        .head = &bb,
        .tail = &bb,
        .registers = {
            .count = 1,
            .list = (IrRegister*[]){&regA}
        },
        .stack_size = 12
    };

    regalloc(
        &func,
        (int[]){0,1,2,3,-1},
        (int[]){-1}
    );

    // Transformed code:
    // - LOADI reg1, 99
    // - LOADSO reg0, 12
    // - STORE32 reg0, reg1
    IrRegister reg0 = {
        .type = REG_REAL,
        .real.index = 0
    };
    IrRegister reg1 = {
        .type = REG_REAL,
        .real.index = 1
    };
    IrInstruction loadi_t = {
        .op = IR_LOADI,
        .dest = &reg1,
        .value = 99
    };
    IrInstruction loadso_t = {
        .op = IR_LOADSO,
        .value = 12,
        .dest = &reg0
    };
    IrInstruction store_t = {
        .op = IR_STORE32,
        .left = &reg0,
        .right = &reg1
    };
    loadi_t.next = &loadso_t;
    loadso_t.next = &store_t;

    IrInstruction * cut = func.head->head->next;
    IrInstruction * expected = &loadi_t;

    while(cut != NULL && expected != NULL)
    {
        assert_true(cut->op == expected->op);

        if(expected->op == IR_LOADI)
        {
            assert_true(cut->value == expected->value);
        }

        compare_reg(cut->dest, expected->dest);
        compare_reg(cut->left, expected->left);
        compare_reg(cut->right, expected->right);

        cut = cut->next;
        expected = expected->next;
    }

    assert_true(cut == NULL && expected == NULL);
}

static void regalloc_fixup_load()
{
    // Code input:
    //  - NOP
    //  - ADD (?), regA, regA
    IrRegister regA = {
        .type = REG_ANY,
        .virtual_index = 1,
        .liveness = {
            .start = 0,
            .finish = 1
        }
    };
    IrInstruction nop = {
        .op = IR_NOP
    };
    IrInstruction add = {
        .op = IR_LOADI,
        .value = 99,
        .dest = NULL,
        .left = &regA,
        .right = &regA
    };
    nop.next = &add;

    // Transformed code:
    //  - LOADSO reg0, 12
    //  - LOAD32 reg2, reg0
    //  - LOADSO reg0, 12
    //  - LOAD32 reg3, reg0
    //  - ADD (?), reg2, reg3
    IrRegister reg0 = {
        .type = REG_REAL,
        .real.index = 2
    };
    IrRegister reg2 = {
        .type = REG_REAL,
        .real.index = 2
    };
    IrRegister reg3 = {
        .type = REG_REAL,
        .real.index = 3
    };
    IrInstruction loadso_t1 = {
        .op = IR_LOADSO,
        .value = 12,
        .dest = &reg0
    };
    IrInstruction load32_t1 = {
        .op = IR_LOAD32,
        .dest = &reg2,
        .left = &reg0
    };
    IrInstruction loadso_t2 = {
        .op = IR_LOADSO,
        .value = 12,
        .dest = &reg0
    };
    IrInstruction load32_t2 = {
        .op = IR_LOAD32,
        .dest = &reg3,
        .left = &reg0
    };
}


int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(regalloc_no_spill),
        cmocka_unit_test(regalloc_no_fixup),
        cmocka_unit_test(regalloc_fixup_store)
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}