#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>

#include "arch.h"

static void arch_basic(void **state)
{
    CType _char = {TYPE_BASIC, .basic.type_specifier = TYPE_UNSIGNED_CHAR};
    assert_true(arch_get_size(&_char) == 1);
    assert_true(arch_get_align(&_char) == 1);
    assert_false(arch_get_signed(&_char));

    CType _short = {TYPE_BASIC, .basic.type_specifier = TYPE_UNSIGNED_SHORT_INT};
    assert_true(arch_get_size(&_short) == 2);
    assert_true(arch_get_align(&_short) == 2);
    assert_false(arch_get_signed(&_short));

    CType _int = {TYPE_BASIC, .basic.type_specifier = TYPE_UNSIGNED_INT};
    assert_true(arch_get_size(&_int) == 4);
    assert_true(arch_get_align(&_int) == 4);
    assert_false(arch_get_signed(&_int));

    CType _long = {TYPE_BASIC, .basic.type_specifier = TYPE_UNSIGNED_LONG_INT};
    assert_true(arch_get_size(&_long) == 4);
    assert_true(arch_get_align(&_long) == 4);
    assert_false(arch_get_signed(&_long));

    CType _signed = {TYPE_BASIC, .basic.type_specifier = TYPE_SIGNED_LONG_INT};
    assert_true(arch_get_signed(&_signed));
}

static void arch_pointer(void **state)
{
    CType _char = {TYPE_BASIC, .basic.type_specifier = TYPE_SIGNED_CHAR};
    CType _ptr = {TYPE_POINTER, .derived.type = &_char};
    assert_true(arch_get_size(&_ptr) == 4);
    assert_true(arch_get_align(&_ptr) == 4);
    assert_false(arch_get_signed(&_ptr));
}

static void arch_array(void **state)
{
    CType _short = {TYPE_BASIC, .basic.type_specifier = TYPE_SIGNED_SHORT_INT};
    CType _array = {TYPE_ARRAY, .derived.type = &_short, .derived.array_size = 12};
    assert_true(arch_get_size(&_array) == 2 * 12);
    assert_true(arch_get_align(&_array) == 2);

    CType _array_array = {TYPE_ARRAY, .derived.type = &_array, .derived.array_size = 3};
    assert_true(arch_get_size(&_array_array) == 2 * 12 * 3);
    assert_true(arch_get_align(&_array_array) == 2);
}

int main(void)
{
    const struct CMUnitTest tests[] = {cmocka_unit_test(arch_basic),
                                       cmocka_unit_test(arch_pointer),
                                       cmocka_unit_test(arch_array)};

    return cmocka_run_group_tests(tests, NULL, NULL);
}