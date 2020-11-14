"""Control statements/expressions functional tests.

This includes:
 - Iteration statements (for, while, do-while) - 6.8.5
 - Jump statements (break, return, continue) - 6.8.6
 - If/else statements - 6.8.4
 - Conditional expressions - 6.5.15
 - Logical And/Or expressions - 6.5.13, 6.5.14
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


@pytest.mark.skip("Not implemented yet")
def test_for(cc):
    assert False


def test_while(cc):
    source = "int a = 0;while(a < 10){a += 1;}return a != 10;"
    cc.body(source)


@pytest.mark.skip("Not implemented yet")
def test_do_while(cc):
    assert False


@pytest.mark.skip("Not implemented yet")
def test_break(cc):
    assert False


def test_return(cc):
    source = "int func(int a, int b, int c) { return a * b * c; }"
    source += "int main() { return func(1,2,3) != 6; }"
    cc.program(source)


@pytest.mark.skip("Not implemented yet")
def test_continue(cc):
    assert False


def test_if(cc):
    cc.body("if(1) return 0; return 1;")
    cc.body("if(1) { return 0; } else { return 1; }")
    cc.body("if(0) return 1; return 0;")
    cc.body("if(0) { return 1; } else { return 0; }")


def test_conditional(cc):
    cc.expression("(1 == 1 ? 2 : 3) == 2")
    cc.expression("(1 == 2 ? 2 : 3) == 3")


@pytest.mark.skip("Not implemented yet")
def test_logical_and(cc):
    source = "int a = 1, b = 11;"
    cc.body(
        source + "if(a++ == 0 && b++) return 1; if(a != 2) return 1;return b != 11;"
    )
    cc.body(
        source
        + "if(a++ == 1 && b++ == 0) return 1; if(a != 2) return 1;return b != 12;"
    )
    cc.body(
        source
        + "if(!(a++ == 1 && b++ == 11)) return 1; if(a != 2) return 1;return b != 12;"
    )


@pytest.mark.skip("Not implemented yet")
def test_logical_or(cc):
    source = "int a = 1, b = 11;"
    cc.body(
        source
        + "if(a++ == 0 || b++ == 0) return 1; if(a != 2) return 1; return b != 12;"
    )
    cc.body(
        source
        + "if(!(a++ == 0 || b++ == 11)) return 1; if (a != 2) return 1; return b != 12;"
    )
    cc.body(
        source + "if(!(a++ == 1 || b++)) return 1; if(a != 2) return 1; return b != 11;"
    )
