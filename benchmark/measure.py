
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

CODE = """
int main()
{
    int fib[200];
    fib[0] = 1;
    fib[1] = 1;
    int i = 2;
    while(i < 200)
    {
        fib[i] = fib[i-1] + fib[i-2];
        i++;
    }
    int total = 0;
    i = 0;
    while(i < 200)
    {
        total += fib[i];
        i++;
    }
    return total;
}
"""

ACC_PATH=os.environ["ACC_PATH"]

def get_codesize(src, compiler):
    with tempfile.NamedTemporaryFile() as fil:
        compiler.compile(src, fil.name)

        elffile = elftools.elf.elffile.ELFFile(fil)
        return len(elffile.get_section_by_name('.text').data())

def get_runtime_performance(src, compiler):
    with tempfile.NamedTemporaryFile() as fil:
        compiler.compile(src, fil.name)

        elffile = elftools.elf.elffile.ELFFile(fil)

        vm = aarch32.Aarch32Vm()
        vm.load_elf(elffile)
        return vm.run(elffile.header['e_entry'])

def test_runtime_performance():
    target_compilers = [
        ArmGccCompiler(None, stdlib=False, opt="-O0"),
        ArmGccCompiler(None, stdlib=False, opt="-O1"),
        ArmGccCompiler(None, stdlib=False, opt="-O2"),
        ArmGccCompiler(None, stdlib=False, opt="-O3"),
        AccAsmCompiler(ACC_PATH, None)
    ]

    results = dict()
    for compiler in target_compilers:
        metrics = get_runtime_performance(CODE, compiler)
        results[str(compiler)] = metrics

    return results

def test_codesize(csv_file):
    target_compilers = [
        ArmGccCompiler(None, stdlib=False, opt="-O0"),
        ArmGccCompiler(None, stdlib=False, opt="-Os"),
        AccAsmCompiler(ACC_PATH, None)
    ]

    results = dict()
    for compiler in target_compilers:
        codesize = get_codesize(CODE, compiler)
        results[str(compiler)] = codesize

    return results

def main():
    codesize_results = test_codesize(os.path.join(os.getcwd(), "codesize.csv"))
    pyplot.subplot(2, 2, 1)
    pyplot.ylabel('Code size (Bytes)')
    pyplot.bar(codesize_results.keys(), codesize_results.values())

    runtime_metrics = test_runtime_performance()

    # Instruction cycles
    pyplot.subplot(2, 2, 2)
    pyplot.ylabel('Code cycles (Instructions)')
    pyplot.bar(runtime_metrics.keys(), [v['cycles'] for v in runtime_metrics.values()])

    # Load instructions
    pyplot.subplot(2, 2, 3)
    pyplot.ylabel('Load instructions')
    pyplot.bar(runtime_metrics.keys(), [v['loads'] for v in runtime_metrics.values()])

    # Store instructions
    pyplot.subplot(2, 2, 4)
    pyplot.ylabel('Store instructions')
    pyplot.bar(runtime_metrics.keys(), [v['stores'] for v in runtime_metrics.values()])

    pyplot.subplots_adjust(wspace=0.4, hspace=0.3)

    fig = pyplot.gcf()
    fig.set_size_inches(7,4)
    fig.savefig(os.path.join(os.path.dirname(__file__), "benchmarks.png"), dpi=200)

main()
