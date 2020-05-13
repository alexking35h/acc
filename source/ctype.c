
#include "ctype.h"

#include <stdio.h>

#include "token.h"

#define TYPE_SIGNEDNESS (TYPE_SIGNED | TYPE_UNSIGNED)
#define TYPE_SPECIFIERS (TYPE_VOID | TYPE_CHAR | TYPE_INT)
#define TYPE_SIZE (TYPE_LONG | TYPE_SHORT)

static void ctype_set_basic_finalise(CType *type, char **err);

void ctype_set_basic_specifier(CType *type, TypeSpecifier type_specifier) {
  type->basic.type_specifier |= type_specifier;
}

void ctype_set_qualifier(CType *type, TypeQualifier type_qualifier) {
  type->type_qualifier |= type_qualifier;
}

void ctype_set_storage_specifier(CType *type, TypeStorageSpecifier storage_specifier) {
  type->storage_class_specifier |= storage_specifier;
}

static void ctype_set_basic_finalise(CType *type, char **error) {
  TypeSpecifier *specifier = &type->basic.type_specifier;
  TypeStorageSpecifier *storage = &type->storage_class_specifier;

  // Report an error if no specifier present.
  if (*specifier == 0) goto err;

  // Check for at most one type-specifier (int/char/void)
  if(((*specifier & TYPE_SPECIFIERS)-1) & (*specifier & TYPE_SPECIFIERS)) 
    goto err;

  // Check for at most one signedness specifier (signed/unsigned)
  if(((*specifier & TYPE_SIGNEDNESS) - 1) & (*specifier & TYPE_SIGNEDNESS))
    goto err;

  // Check for at most one size specifier (short/long)
  if(((*specifier & TYPE_SIZE) - 1) & (*specifier & TYPE_SIZE))
    goto err;

  // Check that at most one storage-class specifier (extern/static) is provided.
  if((*storage - 1) & *storage)
    goto err;

  // Set the default type to 'int', if not specified.
  if (!(*specifier & TYPE_SPECIFIERS)) *specifier |= TYPE_INT;

  // Check void types don't have signed or size specifiers
  if (*specifier & TYPE_VOID) {
    if (*specifier & TYPE_SIGNEDNESS) goto err;
    if (*specifier & TYPE_SIZE) goto err;
  }

  // Char types - should not have size specifiers, and
  // are unsigned by default.
  if (*specifier & TYPE_CHAR) {
    if (*specifier & TYPE_SIZE) goto err;

    if (!(*specifier & TYPE_SIGNEDNESS)) *specifier |= TYPE_UNSIGNED;
  }

  // Int types - signed by default.
  if (*specifier & TYPE_INT) {
    if (!(*specifier & TYPE_SIGNEDNESS)) *specifier |= TYPE_SIGNED;
  }
  return;
err:
  *error = "Invalid type";
}

void ctype_finalise(CType *ctype, char **err) {
  // Finalise CType:
  // - Check type validity
  // - Set primitive type defaults.
  if (ctype->type == TYPE_BASIC) {
    ctype_set_basic_finalise(ctype, err);
    return;
  }
  if (ctype->type == TYPE_FUNCTION) {
    // Check that the child type (the type that this function returns)
    // Is not an array or function type.
    if (ctype->derived.type->type == TYPE_FUNCTION) {
      *err = "Functions cannot return functions (try Python?)";
    } else if (ctype->derived.type->type == TYPE_ARRAY) {
      *err = "Functions cannot return arrays (try Python?)";
    }

    return;
  }
  ctype_finalise(ctype->derived.type, err);
}

void ctype_set_derived(CType *parent, CType *child) {
  parent->derived.type = child;
  child->parent_type = parent;
}

CTypeRank ctype_rank(CType *type) {
  switch (type->basic.type_specifier) {
    case TYPE_SIGNED_CHAR:
    case TYPE_UNSIGNED_CHAR:
      return 1;
    case TYPE_SIGNED_SHORT_INT:
    case TYPE_UNSIGNED_SHORT_INT:
      return 2;
    case TYPE_SIGNED_INT:
    case TYPE_UNSIGNED_INT:
      return 3;
    case TYPE_SIGNED_LONG_INT:
    case TYPE_UNSIGNED_LONG_INT:
      return 4;
    default:
      return 0;
  }
}

int ctype_str(char *buf, int len, const CType* type) {
  if (type->type == TYPE_POINTER) {
    int l = snprintf(buf, len, "pointer to ");
    return l + ctype_str(buf+l, len - l, type->derived.type);
  } else if (type->type == TYPE_ARRAY) {
    int l = snprintf(buf, len, "array of ");
    return l + ctype_str(buf + l, len - l, type->derived.type);
  } else if (type->type == TYPE_FUNCTION) {
    int l = snprintf(buf, len, "function returning ");
    return l + ctype_str(buf + l, len - l, type->derived.type);
  }

  // Must be a basic/arithmetic type.
  int l = 0;
  for(int i=1;i < 256;i <<= 1) {
    TypeSpecifier specifier = (TypeSpecifier)((int)type->basic.type_specifier & i);
    switch(specifier) {
      case TYPE_VOID:
        l += snprintf(buf + l, len - l, "void ");
        break;
      case TYPE_CHAR:
        l += snprintf(buf + l, len - l, "char ");
        break;
      case TYPE_SHORT:
        l += snprintf(buf + l, len - l, "short ");
        break;
      case TYPE_INT:
        l += snprintf(buf + l, len - l, "int ");
        break;
      case TYPE_LONG:
        l += snprintf(buf + l, len - l, "long ");
        break;
      case TYPE_SIGNED:
        l += snprintf(buf + l, len - l, "signed ");
        break;
      case TYPE_UNSIGNED:
        l += snprintf(buf + l, len - l, "unsigned ");
        break;
      default:
        break;
    }
  }
  return l-1;
}