int primary()
{
    if(0)
    {
        return 0;
    }
    return 1;
}

int multiplicative()
{
    if(3*3 != 9) {
        return 0;
    }
    if(6/2 != 3) {
        return 0;
    }
    if(10 % 3 != 1)
    {
        return 0;
    }
    return 1;
}

int additive()
{
    if(9+1 != 10)
    {
        return 0;
    }
    if(9-1 != 8)
    {
        return 0;
    }
    return 1;
}

int bitwise()
{
    if(1 << 1 != 2)
    {
        return 0;
    }
    if(4 >> 2 != 1)
    {
        return 0;
    }
    if(11 & 4)
    {
        return 0;
    }
    if(4 ^ 4)
    {
        return 0;
    }
    if(!(3 ^ 4))
    {
        return 0;
    }
    if(0 | 0)
    {
        return 0;
    }
    return 1;
}

int relational()
{
    if(3 < 1)
    {
        return 0;
    }
    if(10 > 12)
    {
        return 0;
    }
    if(8 >= 9)
    {
        return 0;
    }
    if(255 <= 254)
    {
        return 0;
    }
    return 1;
}

int equality()
{
    if(1 == 0)
    {
        return 0;
    }
    if(2 != 2)
    {
        return 0;
    }
    return 1;
}

int postfix()
{
    // int a = 1;
    // if(a++ != 1)
    // {
    //     return 0;
    // }
    // if(a-- != 2)
    // {
    //     return 0;
    // }
    // if(a != 1)
    // {
    //     return 0;
    // }
    // return 1;
}

int unary()
{
//    int a = 1;
//    if(++a != 2)
//    {
//        return 0;
//    }
//    if(--a != 1)
//    {
//        return 0;
//    }
//    return 1;
}

int tertiary()
{
    if(1 == 2 ? 1 : 0)
    {
        return 0;
    }
    if(1 != 1 ? 1 : 0)
    {
        return 0;
    }
    return 1;
}

int main()
{
    if(!primary())
    {
        return 1;
    }
    if(!multiplicative())
    {
        return 1;
    }
    if(!additive())
    {
        return 1;
    }
    if(!bitwise())
    {
        return 1;
    }
    if(!relational())
    {
        return 1;
    }
    if(!equality())
    {
        return 1;
    }
    if(!tertiary())
    {
        return 1;
    }
    return 0;
}
