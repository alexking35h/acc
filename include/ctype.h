/*
 * C type system implementation.
 *
 * Types in C are divided into:
 *  - Primitive - char, integers, floating types. These are complete
 *    (known size at compile time).
 *  - Derived - arrays, structs, unions, functions,
 *    and pointers.
 *
 * Types are represented with the CType struct. Primitive types are composed
 * of:
 *  - Type specifier - void, char, short, int, long, float, double,
 *    signed, unsigned. The C11 standard regards all valid permutations
 *    of type specifiers as separate types in their own right.
 *  - Type qualifier - const, volatile.
 *  - Storage-class specifier - audo, static, extern, register.
 *
 * Types are represented with the CType struct below. Functions provided for
 * working with primitive types are:
 *  - ctype_set_primitive_specifier()
 *  - ctype_set_primtiive_qualifier()
 *  - ctype_set_primtiive_storage_specifier()
 *  - ctype_set_primitive_finalise()
 *
 * Derived types extend other types (e.g., int* derives from int).
 * Derived types encapsulate a pointer to the CType they derive from;
 * since this can also be a derived type, we have a linked list. E.g.
 * `int **a` looks like:
 *
 * [pointer] -> [ponter] -> [signed int]
 *
 * Functions for working with derived types are:
 *  - ctype_set_derived(parent, child)
 *    Set ctype `parent` to derive from `child`
 *
 * E.g. to create int**
 * > Ctype *prim, *pointer, *ppointer;
 * > ...
 * > ctype_set_derived(pointer, prim);
 * > ctype_set_derived(ppoointer, pointer);
 */

#ifndef __CTYPE__
#define __CTYPE__

#include "token.h"

struct CType;
struct ParameterListItem;

typedef enum {
  TYPE_PRIMITIVE,
  TYPE_ARRAY,
  TYPE_POINTER,
  TYPE_FUNCTION
} CTypeType;

/*
 * Type specifier Enum. This is a bitmask to allow valid
 * permutations of types (e.g., long int). Invalid permutations
 * (short void) are checked in ctype_finalize_primitive_type.
 */
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

typedef struct ParameterListItem {
  Token* name;
  struct CType* type;

  struct ParameterListItem* next;
} ParameterListItem;

typedef struct CType {
  // C Types are either primitive types (arithmetic or pointer),
  // or derived types (pointer, array, function).

  CTypeType type;

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

    // Derived types
    struct {
      struct CType* type;
      struct CType* parent_type;

      int array_size;
      struct ParameterListItem* params;
    } derived;
  };

} CType;

/* Set the type specifier for a type */
void ctype_set_primitive_specifier(CType* type, TypeSpecifier);

/* Set the type qualifier for a type */
void ctype_set_primitive_qualifier(CType* type, TypeQualifier);

/* Set the storage-class specifier for a type */
void ctype_set_primitive_storage_specifier(CType* type, TypeStorageSpecifier);

/*
 * Finalise Primitive C type
 *
 * Validate the C type (e.g., void is not long/short), and set default values
 * (e.g. char -> unsigned char). This functiokn should be called once all
 * type specifiers/qualifiers/storage-specifiers have been parsed.
 */
void ctype_set_primitive_finalise(CType* type);

/*
 * Set `parent` to derive from `child`. (`parent` must not be a primitive type.)
 */
void ctype_set_derived(CType* parent, CType* child);

#endif
