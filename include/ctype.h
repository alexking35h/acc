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
 * > ctype_set_derived(poointer, pointer);
 */

#ifndef __CTYPE__
#define __CTYPE__

struct CType;
struct ParameterListItem;

#include <stdbool.h>

#include "token.h"

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
  TYPE_STATIC = 2,
  TYPE_AUTO = 4,
  TYPE_REGISTER = 8
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
      int array_size;
      struct ParameterListItem* params;
    } derived;
  };

  struct CType* parent_type;

} CType;

/* Set the type specifier for a type */
void ctype_set_primitive_specifier(CType* type, TypeSpecifier);

/* Set the type qualifier for a type */
void ctype_set_primitive_qualifier(CType* type, TypeQualifier);

/* Set the storage-class specifier for a type */
void ctype_set_primitive_storage_specifier(CType* type, TypeStorageSpecifier);

/*
 * Finalise C Type
 *
 *  Validate the C type (e.g., void is not long/short), set default values
 * (e.g., char -> unsigned char), and check derived types (e.g., functions
 * do not derive from functions). This function should be called once all
 * type specifiers/qualifiers and declarators have been parsed.
 *
 * This function sets the err char pointer if an error occurs, otherwise the
 * type is valid.
 */
void ctype_finalise(CType*, char**);

/*
 * Set `parent` to derive from `child`. (`parent` must not be a primitive type.)
 */
void ctype_set_derived(CType* parent, CType* child);

#endif
