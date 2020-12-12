
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "token.h"
#include "ctype.h"

#define TYPE_SIGNEDNESS (TYPE_SIGNED | TYPE_UNSIGNED)
#define TYPE_SPECIFIERS (TYPE_VOID | TYPE_CHAR | TYPE_INT)
#define TYPE_SIZE (TYPE_SHORT)

static void ctype_set_basic_finalise(CType *type, char **err);

void ctype_set_basic_specifier(CType *type, TypeSpecifier type_specifier)
{
    type->basic.type_specifier |= type_specifier;
}

void ctype_set_qualifier(CType *type, TypeQualifier type_qualifier)
{
    type->type_qualifier |= type_qualifier;
}

void ctype_set_storage_specifier(CType *type, TypeStorageSpecifier storage_specifier)
{
    type->storage_class_specifier |= storage_specifier;
}

static void ctype_set_basic_finalise(CType *type, char **error)
{
    TypeSpecifier *specifier = &type->basic.type_specifier;
    TypeStorageSpecifier *storage = &type->storage_class_specifier;

    // Report an error if no specifier present.
    if (*specifier == 0)
        goto err;

    // Check for at most one type-specifier (int/char/void)
    if (((*specifier & TYPE_SPECIFIERS) - 1) & (*specifier & TYPE_SPECIFIERS))
        goto err;

    // Check for at most one signedness specifier (signed/unsigned)
    if (((*specifier & TYPE_SIGNEDNESS) - 1) & (*specifier & TYPE_SIGNEDNESS))
        goto err;

    // Check that at most one storage-class specifier (extern/static) is provided.
    if ((*storage - 1) & *storage)
        goto err;

    // Set the default type to 'int', if not specified.
    if (!(*specifier & TYPE_SPECIFIERS))
        *specifier |= TYPE_INT;

    // Check void types don't have signed or size specifiers
    if (*specifier & TYPE_VOID)
    {
        if (*specifier & TYPE_SIGNEDNESS)
            goto err;
        if (*specifier & TYPE_SIZE)
            goto err;
    }

    // Char types - should not have size specifiers, and
    // are unsigned by default.
    if (*specifier & TYPE_CHAR)
    {
        if (*specifier & TYPE_SIZE)
            goto err;

        if (!(*specifier & TYPE_SIGNEDNESS))
            *specifier |= TYPE_UNSIGNED;
    }

    // Int types - signed by default.
    if (*specifier & TYPE_INT)
    {
        if (!(*specifier & TYPE_SIGNEDNESS))
            *specifier |= TYPE_SIGNED;
    }
    return;
err:
    *error = "Invalid type";
}

void ctype_finalise(CType *ctype, char **err)
{
    // Finalise CType:
    // - Check type validity
    // - Set primitive type defaults.
    if (ctype->type == TYPE_BASIC)
    {
        ctype_set_basic_finalise(ctype, err);
        return;
    }
    if (ctype->type == TYPE_FUNCTION)
    {
        // Check that the child type (the type that this function returns)
        // Is not an array or function type.
        if (ctype->derived.type->type == TYPE_FUNCTION)
        {
            *err = "Functions cannot return functions (try Python?)";
        }
        else if (ctype->derived.type->type == TYPE_ARRAY)
        {
            *err = "Functions cannot return arrays (try Python?)";
        }

        return;
    }
    ctype_finalise(ctype->derived.type, err);
}

void ctype_set_derived(CType *parent, CType *child)
{
    parent->derived.type = child;
    child->parent_type = parent;
}

CTypeRank ctype_rank(CType *type)
{
    switch (type->basic.type_specifier)
    {
    case TYPE_SIGNED_CHAR:
        return 1;
    case TYPE_UNSIGNED_CHAR:
        return 2;

    case TYPE_SIGNED_SHORT_INT:
        return 3;
    case TYPE_UNSIGNED_SHORT_INT:
        return 4;

    case TYPE_SIGNED_INT:
        return 5;
    case TYPE_UNSIGNED_INT:
        return 6;
    default:
        return 0;
    }
}

char *ctype_str(const CType *type)
{
    char *buf = calloc(250, sizeof(char));
    int len = 0;

    while (1)
    {
        if (type->type == TYPE_POINTER)
        {
            len += snprintf(buf + len, 250 - len, "pointer to ");
            type = type->derived.type;
        }
        else if (type->type == TYPE_ARRAY)
        {
            len += snprintf(buf + len, 250 - len, "array of ");
            type = type->derived.type;
        }
        else if (type->type == TYPE_FUNCTION)
        {
            len += snprintf(buf + len, 250 - len, "function returning ");
            type = type->derived.type;
        }
        else
        {
            // Must be a basic/arithmetic type.
            for (int i = 1; i < 256; i <<= 1)
            {
                TypeSpecifier specifier =
                    (TypeSpecifier)((int)type->basic.type_specifier & i);
                switch (specifier)
                {
                case TYPE_VOID:
                    len += snprintf(buf + len, 250 - len, "void ");
                    break;
                case TYPE_CHAR:
                    len += snprintf(buf + len, 250 - len, "char ");
                    break;
                case TYPE_SHORT:
                    len += snprintf(buf + len, 250 - len, "short ");
                    break;
                case TYPE_INT:
                    len += snprintf(buf + len, 250 - len, "int ");
                    break;
                case TYPE_SIGNED:
                    len += snprintf(buf + len, 250 - len, "signed ");
                    break;
                case TYPE_UNSIGNED:
                    len += snprintf(buf + len, 250 - len, "unsigned ");
                    break;
                default:
                    break;
                }
            }
            break;
        }
    }
    buf[len - 1] = '\0';
    return buf;
}

bool ctype_eq(CType *a, CType *b)
{
    if(a->type != b->type)
    {
        return false;
    }
    if(a->type == TYPE_BASIC)
    {
        return a->basic.type_specifier == b->basic.type_specifier;
    }
    else if(a->type == TYPE_ARRAY || a->type == TYPE_POINTER)
    {
        return ctype_eq(a->derived.type, b->derived.type);
    }
    else
    {
        if(!ctype_eq(a->derived.type, b->derived.type)) return false;

        ParameterListItem * a_param = a->derived.params;
        ParameterListItem * b_param = b->derived.params;

        while(a_param && b_param)
        {
            if(!ctype_eq(a_param->type, b_param->type)) return false;

            a_param = a_param->next;
            b_param = b_param->next;
        }

        return a_param == NULL && b_param == NULL;
    }
}

/*
 * Check if two pointers are compatible
 */
_Bool ctype_pointers_compatible(CType *a, CType *b)
{
    while (CTYPE_IS_POINTER(a) && CTYPE_IS_POINTER(b))
    {
        a = a->derived.type;
        b = b->derived.type;
    }
    return CTYPE_IS_BASIC(a) && CTYPE_IS_BASIC(b);
}