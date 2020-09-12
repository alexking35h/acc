#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>

#include <cmocka.h>

static void liveness_basic_block(void ** state)
{
    // Test Liveness analysis on a single basic block.
    assert_true(false);
}

static void liveness_loop(void ** state)
{
    // Test Liveness analysis on a more complex CFG
    assert_true(false);
}

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(liveness_basic_block),
        cmocka_unit_test(liveness_loop)
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}