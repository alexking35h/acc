int additive()
{
    // int a[2];
    // a[0] = 13;
    // a[1] = 14;
    // if(*(&a[0] + 1) != 14) return 0;
    // if(*(&a[1] - 1) != 13) return 0;

    // short b[2];
    // b[0] = 7;
    // b[1] = 5;
    // if(*(&b[0] + 1) != 5) return 0;
    // if(*(&b[1] - 1) != 7) return 0;

    // char c[2];
    // c[0] = 19;
    // c[1] = 18;
    // if(*(&c[0] + 1) != 18) return 0;
    // if(*(&c[1] - 1) != 19) return 0;

    return 1;
}

int unary()
{
    // int a[2];
    // a[0] = 5;
    // a[1] = 4;
    // int * ap = a;
    // if(*(++ap) != 4) return 0;
    // if(*(--ap) != 5) return 0;

    // short b[2];
    // b[0] = 9;
    // b[1] = 8;
    // short * bp = b;
    // if(*(++bp) != 8) return 0;
    // if(*(--bp) != 9) return 0;

    char c[2];
    c[0] = 13;
    c[1] = 11;
    char * cp = c;
    if(*(++cp) != 11) return 0;
    if(*(--cp) != 13) return 0;

    return 1;
}

int postfix()
{
    // int a[3];
    // a[0] = 11;
    // a[1] = 12;
    // int * ap = a;
    // if(*(ap++) != 11) return 0;
    // if(*(ap--) != 12) return 0;
    // if(*ap != 11) return 0;

    // short b[3];
    // b[0] = 14;
    // b[1] = 15;
    // short * bp = b;
    // if(*(bp++) != 14) return 0;
    // if(*(bp--) != 15) return 0;
    // if(*bp != 14) return 0;

    char c[3];
    c[0] = 21;
    c[1] = 22;
    char * cp = c;
    if(*(cp++) != 21) return 0;
    if(*(cp--) != 22) return 0;
    if(*cp != 21) return 0;

    return 1;
}

int main()
{
    if(!additive()) return 1;
    if(!unary()) return 1;
    if(!postfix()) return 1;

    return 0;
}