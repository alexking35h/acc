// Integer-arithmetic compiler tests:
// Binary:
// - additive (+,-)
// - multiplicative (*,/,%)
// - bitwise ()
// Unary:
// - increment and decrement (++, --)
// - negative (-)
// Postfix:
// - increment and decrement (++, --)
// Assignment
// - Including assignment-expressions (+=, -=, *=, /=, %=, ^=, |=, &=)

int multiplicative()
{
    if((3*3) != 9) return 0;
    if((12/4) != 3) return 0;
    if((10 % 3) != 1) return 0;

    return 1;
}
int additive()
{
    if((9 + 1) != 10) return 0;
    if((9 - 10) != -1) return 0;

    return 1;
}
int bitwise()
{
    if((1 << 1) != 2) return 0;
    if((4 >> 2) != 1) return 0;
    if((11 & 4) != 0) return 0;
    if((4 ^ 4) != 0) return 0;
    if((3 ^ 4) != 7) return 0;
    if((0 | 0) != 0) return 0;

    return 1;
}
int unary()
{
    int a = 12;
    if(++a != 13) return 0;
    if(++a != 14) return 0;

    int b = 15;
    if(--b != 14) return 0;
    if(--b != 13) return 0;

    if((-13) != -13) return 0;
    if(~(-2) != 1) return 0;
    if(+(-4) != -4) return 0;

    return 1;
}
int postfix()
{
    int a = 19;
    if(a++ != 19) return 0;
    if(a++ != 20) return 0;

    int b = 99;
    if(b-- != 99) return 0;
    if(b-- != 98) return 0;

    return 1;
}
int assignment()
{
    int a, b;
    if((a = 2) != 2) return 0;
    if(a != 2) return 0;

    if((a += 15) != 17) return 0;
    if(a != 17) return 0;

    if((a -= 15) != 2) return 0;
    if(a != 2) return 0;

    if((a *= 9) != 18) return 0;
    if(a != 18) return 0;

    if((a %= 12) != 6) return 0;
    if(a != 6) return 0;

    if((a /= 2) != 3) return 0;
    if(a != 3) return 0;

    if((a ^= 8) != 11) return 0;
    if(a != 11) return 0;

    if((a |= 4) != 15) return 0;
    if(a != 15) return 0;

    if((a &= 4) != 4) return 0;
    if(a != 4) return 0;

    return 1;
}

int main()
{
    if(!multiplicative()) return 1;
    if(!additive()) return 1;
    if(!bitwise()) return 1;
    if(!unary()) return 1;
    if(!postfix()) return 1;
    if(!assignment()) return 1;
    
    return 0;
}
