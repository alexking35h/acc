#ifndef __CTYPE__
#define __CTYPE__

typedef enum {
  TYPE_VOID = 1,
  TYPE_CHAR = 2,
  TYPE_SHORT = 4,
  TYPE_INT = 8,
  TYPE_LONG = 16,
  TYPE_FLOAT = 32,
  TYPE_DOUBLE = 64,
  TYPE_SIGNED = 128,
  TYPE_UNSIGNED = 256
} TypeSpecifier;

typedef enum { TYPE_CONST = 1, TYPE_VOLATILE } TypeQualifier;

typedef enum {
  TYPE_EXTERN = 1,
  TYPE_STATIC,
  TYPE_AUTO,
  TYPE_REGISTER
} TypeStorageSpecifier;

typedef struct CType {
  // C Types are either scaller types (arithmetic or pointer),
  // or aggregate (array, struct, union).

  enum { TYPE_PRIMITIVE, TYPE_ARRAY, TYPE_POINTER } type;

  union {
    // primitive arithmetic data types
    struct {
      // Type specifier
      TypeSpecifier type_specifier;

      // Type Qualifier
      TypeQualifier type_qualifier;

      // Storage class specifier
      TypeStorageSpecifier storage_class_specifier;
    } primitive;

    // Array data type.
    struct {
      struct CType* type;
      int size;
    } array;

    // Pointer data type.
    struct {
      struct CType* target;
    } pointer;
  };

} CType;

void ctype_set_primitive_type(CType* type, TypeSpecifier, TypeQualifier,
                              TypeStorageSpecifier);

void ctype_finalize_primitive_type(CType* type);

#endif
