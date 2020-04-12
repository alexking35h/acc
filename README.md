[![](https://github.com/alexking35h/acc/workflows/Build%20and%20Test/badge.svg)](https://github.com/alexking35h/acc/actions?query=workflow%3A%22Build+and+Test%22)

# Alex's C Compiler

Welcome! ACC is a work-in-progress implementation of a C11 compiler, written in C.
This is a hobby project I started to brush up on C, and learn more about compilers.

The goals of ACC are:
 * Hand-coded everything, including scanner/lexer and recursive-descent parser
 * Well documented design and implementation
 * Thoroughly tested

Optimizations are a nice-to-have, as is portability: ACC will support only
one platform - probably x86, and is not designed with extensibility/porting in mind.

## Design

The compiler is split into the front-end and back-end, which focus on the reading the
source language and generating the output code respectively. The components that make 
up the compiler are listed below, including design & implementation notes.

|Component                      |Notes|
|-------------------------------|-----|
|Scanner                        |Generate a flat sequence of tokens from the source input.|
|[Parser](design/parser.md)     |Generate the Abstract Syntax Tree (AST) from the output of the Scanner. This includes parsing type declarations|

## Missing

Generally speaking, I've implemented language features depth-first in the front-end (how very Agile!), 
adding support for incremental subsets of the grammar in the parser and type system. Unfortunately,
this means it's missing some (many) features of the C11 grammar, which really makes it closer to C99. Let's call it C--.

### Scanner

 * `_Thread_local`, `_Noreturn`, `_Alignas`, `_Atomic`, `_Bool`, `_Complex`, `_Imaginary` type-specifiers are not implemented.
   The first four were both new additions in C11, the others were added in C99.

 * `_Alignof`, `_Generic`, `_Static_assert`, and `__func__` are not implemented. (All were added in C11.)

