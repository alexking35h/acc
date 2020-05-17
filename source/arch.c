#include "arch.h"
#include "ctype.h"

int arch_get_size(CType* type) {
    if(CTYPE_IS_BASIC(type)) {
        switch(type->basic.type_specifier) {
            case TYPE_UNSIGNED_CHAR:
            case TYPE_SIGNED_CHAR:
                return 1;
            case TYPE_UNSIGNED_SHORT_INT:
            case TYPE_SIGNED_SHORT_INT:
                return 2;
            case TYPE_UNSIGNED_INT:
            case TYPE_SIGNED_INT:
                return 4;
            case TYPE_UNSIGNED_LONG_INT:
            case TYPE_SIGNED_LONG_INT:
                return 8;
        }
    } else if(CTYPE_IS_POINTER(type)) {
        return 4;
    }
    return 0;
}

int arch_get_alignment(CType* type) {
    if(CTYPE_IS_BASIC(type)) {
        switch(type->basic.type_specifier) {
            case TYPE_UNSIGNED_CHAR:
            case TYPE_SIGNED_CHAR:
                return 1;
            case TYPE_UNSIGNED_SHORT_INT:
            case TYPE_SIGNED_SHORT_INT:
                return 2;
            case TYPE_UNSIGNED_INT:
            case TYPE_SIGNED_INT:
                return 4;
            case TYPE_UNSIGNED_LONG_INT:
            case TYPE_SIGNED_LONG_INT:
                return 4;
        }
    } else if(CTYPE_IS_POINTER(type)) {
        return 4;
    }
    return 0;
}