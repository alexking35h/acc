"""Assorted algorithms to verify end-to-end compiler functionality.

These tests include:
 - Sum of array of integers
 - Recursive Fibonacci sum
"""

import pytest
import tempfile
import functools
import os

from acctools import compilers

ACC_PATH=os.environ.get("ACC_PATH", os.path.join(os.path.dirname(__file__), "../build/acc"))

COMPILERS = [
    compilers.GccCompiler,
    functools.partial(compilers.AccIrCompiler, ACC_PATH, regalloc=False),
    functools.partial(compilers.AccIrCompiler, ACC_PATH, regalloc=True),
    functools.partial(compilers.AccAsmCompiler, ACC_PATH)
]


@pytest.fixture(params=COMPILERS)
def cc(request):
    with tempfile.NamedTemporaryFile() as temp_file:
        return request.param(temp_file.name)


CALCULATE_SUM = """
int calc_sum(unsigned char * arr, int n)
{
    int i = 0, tot = 0;
    while(i < n)
    {
        tot += arr[i++];
    }
    return tot;
}
int main()
{
    unsigned char arr[7];
    arr[0] = 1;
    arr[1] = 2;
    arr[2] = 4;
    arr[3] = 8;
    arr[4] = 16;
    arr[5] = 32;
    arr[6] = 64;

    return calc_sum(arr, 7);
}
"""


def test_calculate_array_sum(cc):
    cc.program(CALCULATE_SUM, returncode=127);


FIBONACCI = """
int fib(int n)
{
    if(n == 0) return 1;
    if(n == 1) return 1;
    return fib(n-1) + fib(n-2);
}
int main()
{
    int i = 0, tot = 0;
    while(i < 10)
    {
        tot += fib(i);
        i++;
    }
    return tot;
}
"""


def test_fibonacci(cc):
    cc.program(FIBONACCI, returncode=143)


INSERTION_SORT = """
int sort(int * arr, int n)
{
    int i = 1;
    while(i < n)
    {
        int t = arr[i];
        int j = i - 1;
        while((t < arr[j]) & (j != -1))
        {
            arr[j+1] = arr[j];
            j--;
        }
        arr[j+1] = t;
        i++;
    }
}
int main()
{
    int l[10];
    l[0] = 3;
    l[1] = 8;
    l[2] = 9;
    l[3] = 121;
    l[4] = 28;
    l[5] = 1;
    l[6] = 89;
    l[7] = 90;
    l[8] = 104;
    l[9] = 101;

    sort(l, 10);
    return (l[0] != 1) | (l[9] != 121);
}
"""


def test_sort(cc):
    cc.program(INSERTION_SORT)