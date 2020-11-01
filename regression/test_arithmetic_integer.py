"""Integer arithmetic expressions functional tests.

These tests include all arithmetic expressions on integers
(signed/unsigned char, short, and int). In order of preference (6.5):
 - Postfix (6.5.2)
 - Unary (6.5.3)
 - Multiplicative (6.5.5)
 - Additive (6.5.6)
 - Bitwise shift, xor, or, and (6.5.7, 6.5.10, 6.5.11, 12, 13)
 - Relational (6.5.8)
 - Assignment (6.5.16)

Logical operands (&&, ||) are in Control tests (test_control.py), and 
cast expressions are in Conversion tests (test_conversion.py)
"""

import functional
import pytest
import tempfile
import functools
import os

ACC_PATH=os.environ.get("ACC_PATH", os.path.join(os.path.dirname(__file__), "../build/acc"))

COMPILERS = [
    functional.GccCompiler,
    functools.partial(functional.AccIrCompiler, ACC_PATH, regalloc=True),
    functools.partial(functional.AccIrCompiler, ACC_PATH, regalloc=False),
    functools.partial(functional.AccAsmCompiler, ACC_PATH)
]


@pytest.fixture(params=COMPILERS)
def cc(request):
    with tempfile.NamedTemporaryFile() as temp_file:
        return request.param(temp_file.name)


def test_postfix(cc):
    preamble = "int a = 19;"
    cc.body(preamble + "return a++ != 19;")
    cc.body(preamble + "a++;return a++ != 20;")

    cc.body(preamble + "return a-- != 19;")
    cc.body(preamble + "a--; return a-- != 18;")


def test_unary(cc):
    preamble = "int a = 3;"
    cc.body(preamble + "return ++a != 4;")
    cc.body(preamble + "++a; return ++a != 5;")

    cc.body(preamble + "return --a != 2;")
    cc.body(preamble + "--a; return --a != 1;")

    cc.expression("(-13) == -13")
    cc.expression("-(-1) == 1")
    cc.expression("~(-2) == 1")
    cc.expression("+(-4) == -4")


def test_multiplicative(cc):
    cc.expression("(3*3) == 9")
    cc.expression("(12/4) == 3")
    cc.expression("(10 % 3) == 1")


def test_additive(cc):
    cc.expression("(9 + 1) == 10")
    cc.expression("(9 - 10) == -1")


def test_bitwise(cc):
    cc.expression("(1 << 1) == 2")
    cc.expression("(4 >> 2) == 1")
    cc.expression("(11 & 4) == 0")
    cc.expression("(4 ^ 4) == 0")
    cc.expression("(3 ^ 4) == 7")
    cc.expression("(0 | 0) == 0")


@pytest.mark.skip("Not implemented yet.")
def test_relational(cc):
    cc.expression("-1 < 3")
    cc.expression("3 > -1")

    cc.expression("-1 <= 3")
    cc.expression("-1 <= -1")
    cc.expression("5 >= -5")
    cc.expression("5 >= 5")

    cc.expression("1 == 1")
    cc.expression("-5 != 5")


def test_assignment(cc):
    preamble = "int a = 15;"
    operators = [
        ("+=", 20),
        ("-=", 10),
        ("*=", 75),
        ("/=", 3),
        ("%=", 0),
        ("^=", 10),
        ("|=", 15),
        ("&=", 5),
    ]
    for operator, result in operators:
        cc.body(preamble + f"return (a {operator} 5) != {result};")
        cc.body(preamble + f"a {operator} 5;return a != {result};")


def test_precedence(cc):
    """Test expression precedence

    Expression precendence (loosely-bound first):
     - Assignment
     - Conditional
     - Logical OR
     - Logical AND
     - XOR
     - OR
     - AND
     - Equality
     - Relational
     - Shift
     - Additive
     - Multiplicative
     - Cast
     - Unary
     - Postfix
    """
    # Postfix/Unary
    preamble = "int a[1];a[0] = 1;"
    cc.body(preamble + "return -a[0] != -1;")

    # Multiplicative/Additive
    cc.expression("3 * 3 + 4 == 13")

    # Additive/Shift
    cc.expression("2 + 2 << 2 == 16")

    # Shift/ Relational
    cc.expression("2 << 3 < 17")

    # Relational/Equality
    cc.expression("1 < 3 == 1")

    # Equality/And
    cc.expression("(-9 == -9 & 3) == 1")

    # And/Conditional
    cc.expression("(3 & 1 ? 4 : 0) == 4")

    # Everything.
    everything = "int a[1];a[0] = 1;"
    everything += "int b = (13 < ++a[0] * 2 + 3 << 2 == 1 ? 99 : 98);"
    everything += "return b != 99;"
    cc.body(everything)


def test_associativity(cc):
    """Test expression associativity.

    Binary operators are left-associative, assignment is right-associative.
    """
    cc.expression("-1 - 1 - 1 == ((-1 - 1) - 1)")
    cc.expression("5 == 5 == 1")
    cc.body("int a, b;a = b = 4;return a != 4;")
