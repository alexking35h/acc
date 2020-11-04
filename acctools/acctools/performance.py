
import functools

import tempfile
import elftools.elf.elffile

from functional import (
    GccCompiler,
    AccAsmCompiler
)

CODE_BASIC = """
int main()
{
    return 0;
}
"""

CODE_FIBONACCI = """
int fib(int a)
{
    if(a == 1) return 1;
    if(a == 2) return 1;
    return fib(a-1) + fib(a-2);
}
int main()
{
    return fib(500);
}
"""

class PerformanceTest:

    def __init__(self, compiler):
        with tempfile.NamedTemporaryFile() as fil:
            compiler.compile(self.CODE, fil.name)
            self._get_codesize(fil.name)
    
    def _get_cycles(self):
        pass
    
    def _get_codesize(self, elf):
        with open(elf, 'rb') as of:
            elffile = elftools.elf.elffile.ELFFile(of)
            self._codesize = len(elffile.get_section_by_name('.text').data())
    
    @property
    def cycles(self):
        raise NotImplemented
    
    @property
    def codesize(self):
        return self._codesize
        raise NotImplemented


class Basic(PerformanceTest):
    CODE = CODE_BASIC


class Fibonacci(PerformanceTest):
    CODE = CODE_FIBONACCI


class ResultsTable:

    def __init__(self):
        self._results = dict()

    def put(self, compiler, key, result):
        self._results[(str(compiler),key)] = result

    def get_csv(self):
        keys = list(set([k for _,k in self._results.keys()]))
        compilers = list(set([c for c,_ in self._results.keys()]))

        csv = ','.join(['compilers'] + keys)

        for compiler in compilers:
            csv += '\n' + compiler
            for key in keys:
                result = self._results.get((compiler, key), '-')
                csv += ',' + str(result)
        
        return csv

def run_performance_tests(test, results):
    compilers = [
        GccCompiler,
        functools.partial(AccAsmCompiler, "build/acc")
    ]

    for compiler in compilers:
        cc = compiler(None)
        test_results = test(cc)
        results.put(cc, 'code size', test_results.codesize)

def main():
    tests = [
        Basic
    ]

    for test in tests:
        results_table = ResultsTable()
        results = run_performance_tests(test, results_table)
    
        print(results_table.get_csv())

if __name__ == "__main__":
    main()




