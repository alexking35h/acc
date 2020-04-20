
#include "ctype.h"

#include <stdio.h>

#include "token.h"

#define TYPE_SIGNEDNESS (TYPE_SIGNED | TYPE_UNSIGNED)
#define TYPE_SPECIFIERS (TYPE_VOID | TYPE_CHAR | TYPE_INT)
#define TYPE_SIZE (TYPE_LONG | TYPE_SHORT)

static void ctype_set_primitive_finalise(CType *type, char **err);

void ctype_set_primitive_specifier(CType *type, TypeSpecifier type_specifier) {
  type->primitive.type_specifier |= type_specifier;
}

void ctype_set_primitive_qualifier(CType *type, TypeQualifier type_qualifier) {
  type->primitive.type_qualifier |= type_qualifier;
}

void ctype_set_primitive_storage_specifier(CType *type, TypeStorageSpecifier storage_specifier) {
  type->primitive.storage_class_specifier |= storage_specifier;
}

static void ctype_set_primitive_finalise(CType *type, char **error) {
  TypeSpecifier *specifier = &type->primitive.type_specifier;
  TypeStorageSpecifier *storage = &type->primitive.storage_class_specifier;

  // Report an error if no specifier/qualifiers were used.
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
  if (ctype->type == TYPE_PRIMITIVE) {
    ctype_set_primitive_finalise(ctype, err);
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
