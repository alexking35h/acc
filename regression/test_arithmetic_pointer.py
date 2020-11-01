"""Pointer arithmetic functional tests.

These tests include arithmetic expressions on pointers. This
includes:
 - Postfix: ++, -- (6.5.2)
 - Unary: ++, -- (6.5.3)
 - Additive: +, - (6.5.6)
 - Relational: <, <=, >, >= (6.5.8)
 - Assignment: =, +=, -= (6.5.16)
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


@pytest.mark.parametrize("type", ["char", "short", "int"])
def test_postfix(type, cc):
    preamble = f"{type} a[3];a[0] = 11;a[1] = 12;a[2] = 13;{type} * ap = &a[1];"
    cc.body(preamble + "return *ap++ != 12;")
    cc.body(preamble + "ap++;return *ap++ != 13;")

    cc.body(preamble + "return *ap-- != 12;")
    cc.body(preamble + "ap--; return *ap-- != 11;")


@pytest.mark.parametrize("type", ["char", "short", "int"])
def test_unary(type, cc):
    preamble = f"{type} a[3];a[0] = 5;a[1] = 6;a[2] = 7;{type} *ap = &a[1];"
    cc.body(preamble + "return *++ap != 7;")
    cc.body(preamble + "++ap;return *ap != 7;")

    cc.body(preamble + "return *--ap != 5;")
    cc.body(preamble + "--ap;return *ap != 5;")


@pytest.mark.parametrize("type", ["char", "short", "int"])
def test_additive(type, cc):
    preamble = f"{type} a[5];a[0]=22;a[2]=44;a[4]=33;{type} *ap = &a[2];"
    cc.body(preamble + "return *(ap + 2) != 33;")
    cc.body(preamble + "return *(ap - 2) != 22;")


@pytest.mark.skip("Not implemented yet.")
def test_relational(cc):
    assert False


@pytest.mark.parametrize("type", ["char", "short", "int"])
def test_assignment(type, cc):
    preamble = f"{type} a[1];a[0] = 12;{type} * ap = a; {type} * p;"
    cc.body(preamble + f"return *(p = ap) != 12;")
    cc.body(preamble + f"p = ap;return *p != 12;")

    preamble = f"{type} a[2];a[1] = 13;{type} * ap = a;"
    cc.body(preamble + f"return *(ap += 1) != 13;")
    cc.body(preamble + f"ap += 1;return *ap != 13;")

    preamble = f"{type} a[5];a[0] = 15;{type} * ap = &a[4];"
    cc.body(preamble + f"return *(ap -= 4) != 15;")
    cc.body(preamble + f"ap -= 4;return *ap != 15;")
