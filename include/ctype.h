#ifndef __CTYPE__
#define __CTYPE__

typedef struct CType {
  // C Types are either scaller types (arithmetic or pointer),
  // or aggregate (array, struct, union).

  enum { TYPE_PRIMARY, TYPE_ARRAY, TYPE_POINTER } type;

  union {
    // primitive arithmetic data types
    struct {
      // Type specifier
      enum {
        TYPE_VOID,
        TYPE_CHAR,
        TYPE_SHORT,
        TYPE_INT,
        TYPE_LONG,
        TYPE_FLOAT,
        TYPE_DOUBLE,
        TYPE_SIGNED,
        TYPE_UNSIGNED
      } type_specifier;

      // Type Qualifier
      enum { TYPE_CONST, TYPE_VOLATILE } type_qualifier;

      // Storage class specifier
      enum {
        TYPE_TYPEDEF,
        TYPE_EXTERN,
        TYPE_STATIC,
        TYPE_AUTO,
        TYPE_REGISTER
      } storage_class_specifier;
    } primitive;

    // Array data type.
    struct {
      // array data type and size
      struct CType* type;
      int size;
    } array;

    // Pointer data type.
    struct {
      struct CType* target;
    } pointer;
  };

} CType;

#endif
