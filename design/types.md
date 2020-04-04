# Types

The C11 type system lives in the gra

The C11 type system is expressed in the grammar in two places:
 
 * **Declarations**
   Declaring a new variable, function, struct, or union

 * **Type name**
   

The C11 type system lives in the grammar in two places:

 * **Declarations**
   Declaring a new variable, function, struct, or union.

 * **Abstract Declarators**
   Type declarations that do not have an identifier: casts and function 
   prototype parameter parameter lists.



Ignoring the latter, 




   
   Type declarations that do not have an identifier
   

Declarations include a storage-specifier:

 * *Storage-class specifier*: extern, static, auto, and register. These
   affect the location and visibility of a declarator.

 * *Type-specifier*: void, char, short, int, long, float, double, signed, and unsigned.
   These select the type of variable, and vary between architectures.

 * *Type-qualifiers*: const and volatile. These change properties of the declarator.

Declarators then i


   
