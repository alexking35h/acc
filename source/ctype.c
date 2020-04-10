
#include "ctype.h"

#define TYPE_SIGNEDNESS (TYPE_SIGNED | TYPE_UNSIGNED)
#define TYPE_SPECIFIERS (TYPE_VOID | TYPE_CHAR | TYPE_INT)
#define TYPE_SIZE (TYPE_LONG | TYPE_SHORT)

void ctype_set_primitive_specifier(CType *type, TypeSpecifier type_specifier) {
  TypeSpecifier *specifier = &type->primitive.type_specifier;

  // Signed-ness
  if (type_specifier & TYPE_SIGNEDNESS) {
    // Check that the signed-ness has not already been specified.
    if (*specifier & TYPE_SIGNEDNESS) goto err;

    *specifier |= (TYPE_SIGNEDNESS & type_specifier);
  }

  // Type
  if (type_specifier & TYPE_SPECIFIERS) {
    // Check that the type has not already been specified
    if (*specifier & TYPE_SPECIFIERS) goto err;

    *specifier |= (TYPE_SPECIFIERS & type_specifier);
  }

  // Size (short/long)
  if (type_specifier & TYPE_SIZE) {
    // Check that the size has not already been specified
    if (*specifier & TYPE_SIZE) goto err;

    *specifier |= (TYPE_SIZE & type_specifier);
  }
err:
  return;
}

void ctype_set_primitive_qualifier(CType *type, TypeQualifier type_qualifier) {
  TypeQualifier *qualifier = &type->primitive.type_qualifier;

  // Type qualifiers
  if (type_qualifier != 0) {
    // Check that a qualifier flag has not already been set
    if (*qualifier != 0) goto err;

    *qualifier = type_qualifier;
  }
err:
  return;
}

void ctype_set_primitive_storage_specifier(
    CType *type, TypeStorageSpecifier storage_specifier) {
  TypeStorageSpecifier *storage = &type->primitive.storage_class_specifier;

  // Storage class specifiers
  if (storage_specifier != 0) {
    // Check that a storage class specifier has not already been set
    if (*storage != 0) goto err;

    *storage = storage_specifier;
  }

err:
  return;
}

void ctype_set_primitive_finalise(CType *type) {
  TypeSpecifier *specifier = &type->primitive.type_specifier;

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
err:
  return;
}

void ctype_set_derived(CType *parent, CType *child) {
  parent->derived.type = child;
  child->derived.parent_type = parent;
}
