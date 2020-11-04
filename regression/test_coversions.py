"""Basic/Integer Type conversions functional tests

These tests include all type conversions in C, including:
 - Explicit cast expresssions (6.5.4)
 - Integer promotions (6.3.1.1)
 - Usual arithmetic conversions (6.3.1.8)
"""

import pytest
import tempfile
import functools
import os

from acctools import compilers

ACC_PATH=os.environ.get("ACC_PATH", os.path.join(os.path.dirname(__file__), "../build/acc"))

COMPILERS = [
    compilers.GccCompiler,
    functools.partial(compilers.AccIrCompiler, ACC_PATH, regalloc=True),
    functools.partial(compilers.AccIrCompiler, ACC_PATH, regalloc=False),
    functools.partial(compilers.AccAsmCompiler, ACC_PATH)
]


@pytest.fixture(params=COMPILERS)
def cc(request):
    with tempfile.NamedTemporaryFile() as temp_file:
        return request.param(temp_file.name)


def test_cast_unsigned(cc):
    cc.expression("(unsigned char)1 == 1")
    cc.expression("(unsigned char)257 == 1")
    cc.expression("(unsigned short)65537 == 1")
    cc.expression("(unsigned int)65536 == 65536")
    cc.expression("(unsigned char)-1 == 255")


def test_cast_signed(cc):
    cc.expression("(signed short)(signed char)1 == 1")
    cc.expression("(signed short)(signed char)-1 == -1")
    cc.expression("(signed int)(signed char)1 == 1")
    cc.expression("(signed int)(signed char)-1 == -1")


def test_integer_promotion(cc):
    """Test Integer promotion (6.5.1.4)

    > If an int can represent all values of the original type,
    > the value is converted to an int; otherwise, it is converted
    > to an unsigned int.

    E.g. int a = -a;
     - The unary operator '-' performs integer-promotion on the
       expression 'a', if 'a' is a signed/unsigned char/short.

    Integer promotions are performed:
     - As part of usual arithmetic conversions
     - Unary +, ~, - operands
     - Shift <<, >> expressions.
    """
    preamble = "signed char u8 = 1;signed short u16 = 5;"
    cc.body(preamble + "return ~u8 != -2;")
    cc.body(preamble + "return ~u16 != -6;")

    preamble = "unsigned char u8 = 255;unsigned short u16 = 255;"
    cc.body(preamble + "return -u8 != -255;")
    cc.body(preamble + "return -u16 != -255;")


def test_usual_arithmetic_conversions(cc):
    """Test Usual arithmetic conversions (6.3.1.8)

    > The purpose is to determine a common real type for the operands
    > and result.

    Usual arithmetic conversions encapsulate integer promotions on each operand
    less than int/unsigned int. So for example:
    > (signed char)-1 + (unsigned short)0 => (signed int) + (unsigned int)0.
    """
    # > If both operands have the same type, then no further conversion is needed.
    preamble = "signed int i32_i = -1, i32_j = -2;"
    cc.body(preamble + "return i32_i + i32_j != -3;")

    # > If both operands have signed integer type, or both have unsigned integer
    # > type, the operand with the lesser integer conversion rank is converted to
    # > the operand with the greater rank.
    # (We cannot test in ACC, since all types are <= int/unsigned int.)

    #  > If the operand that has unsigned integer type has greater rank or equal
    # > to the rank of the type of the other operand, then the operand
    # > with the signed integer type is converted to the type of the unsigned
    # > integer type.
    # (We cannot test in ACC, since all types are <= int/unsigned int.)

    # > If the type of the operand with the signed integer type can represent all
    # > values of the type of the operand with the unsigned integer type, then
    # > the operand with unsigned integer type is converted to the type of the
    # > signed integer type.
    # (We cannot test in ACC, since all types are <= int/unsigned int.)

    # > Otherwise, both operands are converted to the unsigned integer type
    # > corresponding to the type of the operand with the signed integer type.
    preamble = "signed int i32 = -1;unsigned int u32 = 0;"
    cc.body(preamble + "return i32 + u32 != 4294967295;")
