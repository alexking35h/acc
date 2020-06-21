/*
 * This C source file contains no syntactic or semantic compiler errors.
 * This is to test that valid C code will pass acc's front-end without error.
 *
 * This is read by the regression test suite (see regression_test.py), and compared with
 * acc's output.
 */

int arithmetic_operators() {
    int q = 1;
    q += (1 + q);
    q -= (1 - q);
    q *= (1 * q);
    q /= (1 / q);
    q ^= (1 ^ q);
    q %= (1 % q);
    q <<= (1 << q);
    q >>= (1 >> q);
}

int * pointer_operations() {
    int q;
    int * x = &q;
    int ** y = &x;

    *x = 12+1;
    **y = *x+1;

    *(x + q) = 12;
    int p = *y - x;

    return x;
}

int array_operations() {
    int q[10];
    q[0] = 12;
    *(q+1) = 12;
    return q;
}