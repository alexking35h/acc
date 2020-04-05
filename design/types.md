# Types

Types in C fall into two categories:

* **Primitive (basic)** - complete, arithmetic types. This includes
  char, int, and floating point.

* **Derived** - compositions of primitive types. This includes
  function prototypes, arrays, structs, unions, and pointers.

## Primitive Types

Primitive type declarations include type specifiers (`char` and `int`);
type qualifiers - which change properties of the type, (`const` and
`volatile`); and storage-class specifiers which change scope and lifetime
behaviour, (`register`, `static`, and `extern`).

Type specifiers/qualifiers are covered by the grammar rule:
```yacc
declaration_specifier:
    : type_specifier declaration_specifier*
    | type_qualifier declaration_specifier*
    | storage_class_specifier declaration_specifier*
```

The standard adds some additional constraints to the grammar:
* At least one type-specifier is required, this can be:
  `void`, `char`, `short`, `int`, `long`, `float`, `double`,
  `signed`, `unsigned`, `_Bool`, `Complex`.
* Type-specifiers can appear in any valid order, provided it is valid.
  I.e. `signed short int` is the same as `int short signed`.
* Declarations should have at most one storage-class specifier
  (with the exception of `_Thread_local`).
