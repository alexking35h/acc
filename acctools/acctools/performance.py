
import functools
import os
import tempfile
import elftools.elf.elffile

from matplotlib import pyplot

from acctools.compilers import (
    ArmGccCompiler,
    AccAsmCompiler
)

from acctools import aarch32

CODE_BASIC = """
int main()
{
    return 0;
}
"""

CODE_LOOP = """
int main()
{
    int a = 0;
    while(a < 1000)
    {
        a = a + 1;
    }
    return a;
}
"""

CODE_FIBONACCI = """
int fib(int a)
{
    if(a == 1) return 1;
    if(a == 2) return 1;
    return fib(a-1) + fib(a-2);
}
int _start()
{
    return fib(10);
}
"""

ACC_PATH=os.environ.get("ACC_PATH", os.path.join(os.path.dirname(__file__), "../..//build/acc"))

def get_codesize(src, compiler):
    with tempfile.NamedTemporaryFile() as fil:
        compiler.compile(src, fil.name)

        elffile = elftools.elf.elffile.ELFFile(fil)
        return len(elffile.get_section_by_name('.text').data())

def get_cycles(src, compiler):
    with tempfile.NamedTemporaryFile() as fil:
        compiler.compile(src, fil.name)

        elffile = elftools.elf.elffile.ELFFile(fil)

        vm = aarch32.Aarch32Vm()
        vm.load_elf(elffile)
        return vm.run(elffile.header['e_entry'])

def test_cycles():
    target_compilers = [
        ArmGccCompiler(None, stdlib=False, opt="-O0"),
        ArmGccCompiler(None, stdlib=False, opt="-O1"),
        AccAsmCompiler(ACC_PATH, None)
    ]

    results = dict()
    for compiler in target_compilers:
        cycles = get_cycles(CODE_LOOP, compiler)
        results[str(compiler)] = cycles

    return results

def test_codesize(csv_file):
    target_compilers = [
        ArmGccCompiler(None, stdlib=False, opt="-O0"),
        ArmGccCompiler(None, stdlib=False, opt="-Os"),
        AccAsmCompiler(ACC_PATH, None)
    ]

    results = dict()
    for compiler in target_compilers:
        codesize = get_codesize(CODE_BASIC, compiler)
        results[str(compiler)] = codesize

    return results

def main():
    codesize_results = test_codesize(os.path.join(os.getcwd(), "codesize.csv"))
    pyplot.subplot(2, 1, 1)
    pyplot.ylabel('Code size (Bytes)')
    pyplot.bar(codesize_results.keys(), codesize_results.values())

    cycles_results = test_cycles()
    pyplot.subplot(2, 1, 2)
    pyplot.ylabel('Code cycles (Instructions)')
    pyplot.bar(cycles_results.keys(), cycles_results.values())

    pyplot.savefig(os.path.join(os.getcwd(), "performance.png"))

main()
