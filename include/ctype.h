/*
 * C type system implementation.
 * 
 * Types in C are divided into:
 *  - Basic - char, integers, floating types. These are complete objects
 *    (known size at compile time).
 *  - Derived - arrays, structs, unions, functions, and pointers.
 * 
 * All types can also have qualifiers (const, volatile, and restrict), and 
 * storage-class specifiers (register, extern, static, and auto).
 * 
 * Types are represented with the CType struct. Basic types are composed of:
 *  - Type specifier - char, short, int, long, signed, unsigned, etc. Valid permutations
 *    of the same type specifiers are considered equal. E.g., 'signed int' = 'int signed'.
 * 
 * Functions for working with basic types are:
 *  - ctype_set_basic_specifier()
 * 
 * Derived types extend other types (e.g. int* derives from int). Derived types
 * include a pointer to the CType they derive from; since this can also be a 
 * derived type, we have a linked list. E.g. `int **a` looks like:
 * 
 * [pointer] -> [pointer] -> [signed int]
 * 
 * Functions for working with derived types are:
 *  - ctype_set_derived(parent, child)
 * 
 * Types qualifiers and storage-class specifiers are set with the function:
 *  - ctype_set_storage_specifier()
 *  - ctype_set_qualifier()
 * 
 * E.g., to create static int*
 * > CType *basic, *pointer, **ppointer;
 * > ctype_set_storage_specifier(basic, TYPE_STATIC);
 * > ctype_set_derived(pointer, basic);
 * > ctype_set_derived(ppointer, pointer);
 */

#ifndef __CTYPE__
#define __CTYPE__

struct CType;
struct ParameterListItem;

#include <stdbool.h>

#include "token.h"

typedef enum {
  TYPE_BASIC,
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
  TYPE_SIGNED = 128,
  TYPE_UNSIGNED = 256,

  // Actual types
  TYPE_SIGNED_CHAR = TYPE_CHAR | TYPE_SIGNED,
  TYPE_UNSIGNED_CHAR = TYPE_CHAR | TYPE_UNSIGNED,
  TYPE_SIGNED_SHORT_INT = TYPE_SIGNED | TYPE_SHORT | TYPE_INT,
  TYPE_UNSIGNED_SHORT_INT = TYPE_UNSIGNED | TYPE_SHORT | TYPE_INT,
  TYPE_SIGNED_INT = TYPE_SIGNED | TYPE_INT,
  TYPE_UNSIGNED_INT = TYPE_UNSIGNED | TYPE_INT,
  TYPE_SIGNED_LONG_INT = TYPE_SIGNED | TYPE_LONG | TYPE_INT,
  TYPE_UNSIGNED_LONG_INT = TYPE_UNSIGNED | TYPE_LONG | TYPE_INT
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
  CTypeType type;

  TypeQualifier type_qualifier;
  TypeStorageSpecifier storage_class_specifier;
  
  union {
    // basic types
    struct {
      TypeSpecifier type_specifier;
    } basic;

    // Derived types
    struct {
      struct CType* type;
      int array_size;
      struct ParameterListItem* params;
    } derived;
  };

  struct CType* parent_type;
} CType;

/* Set the type specifier for a basic type */
void ctype_set_basic_specifier(CType*, TypeSpecifier);

/* Set the qualifier and storage-class specifier for a type. */
void ctype_set_qualifier(CType*, TypeQualifier);
void ctype_set_storage_specifier(CType*, TypeStorageSpecifier);

/*
 * Finalise C Type
 *
 * Validate the C type (e.g., void is not long/short), set default values
 * (e.g., char -> unsigned char), and check derived types (e.g., functions
 * do not derive from functions). This function should be called once all
 * type specifiers/qualifiers and declarators have been parsed.
 *
 * This function sets the err char pointer if an error occurs, otherwise the
 * type is valid.
 */
void ctype_finalise(CType* type, char** error_str);

/* Set `parent` to derive from `child`. (`parent` must not be a primitive type.) */
void ctype_set_derived(CType* parent, CType* child);

/* Get the type rank (see 6.3.1) for a given C Type */
typedef unsigned char CTypeRank;
CTypeRank ctype_rank(CType *type);

/* 
 * Get the user-friendly name for a type.
 * E.g. `int *` -> `pointer to signed int`
 */
char *ctype_str(const CType* type);

/*
 * Check if two pointers are compatible.
 */
_Bool ctype_pointers_compatible(CType* a, CType* b);

/*
 * Macro definitions
 */
#define CTYPE_IS_BASIC(C) (C->type == TYPE_BASIC)
#define CTYPE_IS_FUNCTION(C) (C->type == TYPE_FUNCTION)
#define CTYPE_IS_POINTER(C) (C->type == TYPE_POINTER)
#define CTYPE_IS_SCALAR(C) (CTYPE_IS_BASIC(C) || CTYPE_IS_POINTER(C))
#define CTYPE_IS_ARRAY(C) (C->type == TYPE_ARRAY)

#endif
