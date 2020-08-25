int cast_unsigned()
{
    if((unsigned char)1 != 1)
    {
        return 0;
    }
    if((unsigned char)257 != 1)
    {
        return 0;
    }
    if((unsigned short)65537 != 1)
    {
        return 0;
    }
    if((unsigned int)65536 != 65536)
    {
        return 0;
    }
    if((unsigned char)-1 != 255)
    {
        return 0;
    }
    return 1;
}
int cast_signed()
{
    signed char a = 1;
    signed char b = -1;
    if((signed short)a != 1)
    {
        return 0;
    }
    if((signed short)b != -1)
    {
        return 1;
    }
    if((signed int)a != 1)
    {
        return 1;
    }
    if((signed int)b != -1)
    {
        return 1;
    }
    return 1;
}
int main()
{
    if(!cast_unsigned())
    {
        return 1;
    }
    if(!cast_signed())
    {
        return 1;
    }
    return 0;
}
