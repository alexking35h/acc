#include "arch.h"

typedef struct ArchTypeLookup
{
    TypeSpecifier type;
    int size;
    int align;
} ArchTypeLookup;

ArchTypeLookup arch_type[] = {
    {TYPE_LONG, 4, 4},
    {TYPE_SHORT, 2, 2},
    {TYPE_CHAR, 1, 1},
    {TYPE_INT, 4, 4},
    {0, 0, 0}
};

int arch_get_size(CType * ctype)
{
    if(CTYPE_IS_BASIC(ctype)) {
        for(ArchTypeLookup * arch = arch_type;arch->type != 0;arch++) 
        {
            if(arch->type & ctype->basic.type_specifier)
                return arch->size;
        }
    } else if(CTYPE_IS_POINTER(ctype)) {
        return 4;
    } else if(CTYPE_IS_ARRAY(ctype)) {
        return arch_get_size(ctype->derived.type) * ctype->derived.array_size;
    }
    return -1;
}

int arch_get_align(CType * ctype) {
    if(CTYPE_IS_BASIC(ctype)) {
        for(ArchTypeLookup * arch = arch_type;arch->type != 0;arch++) 
        {
            if(arch->type & ctype->basic.type_specifier)
                return arch->align;
        }
    } else if(CTYPE_IS_POINTER(ctype)) {
        return 4;
    } else if(CTYPE_IS_ARRAY(ctype)) {
        return arch_get_align(ctype->derived.type);
    }
    return -1;
}

bool arch_get_signed(CType * ctype)
{
    if(CTYPE_IS_BASIC(ctype)) {
        return ctype->basic.type_specifier & TYPE_SIGNED;
    }
    return false;
}