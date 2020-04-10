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
|                               |
