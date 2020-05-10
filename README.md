[![](https://github.com/alexking35h/acc/workflows/Build%20and%20Test/badge.svg)](https://github.com/alexking35h/acc/actions?query=workflow%3A%22Build+and+Test%22) [![](https://github.com/alexking35h/acc/workflows/Regression%20Test/badge.svg)](https://github.com/alexking35h/acc/actions?query=workflow%3A%22Regression+Test%22)

# Alex's C Compiler

Welcome! ACC is a work-in-progress implementation of a C11 compiler, written in C.
This is a hobby project I started to brush up on C, and learn more about compilers.

The goals of ACC are:
 * Hand-coded everything, including scanner and parser
 * Documented design and implementation
 * Thoroughly tested

Optimizations are a nice-to-have, as is portability: ACC will support only
one platform - probably x86, and is not designed with extensibility/porting in mind.

## Design

The compiler is split into separate components which marshall the program from source, to
intermediate, to machine code. 

| Component | Header File | Notes |
|-----------|-------------|-------|
| Scanner | [scanner.h](include/scanner.h) | Generate a flat sequence of tokens from source input. |
| [Parsing](design/parser.md) | [parser.h](incude/parser.h) | Generate the AST from the output of the scanner. | 
| Context-sensitive Analysis | [analysis.h](include/analysis.h) | Annotate the AST with type information, catch semantic errors, and handle type conversions |

Seperately, parts of the C language are implemented independently to support compilation.

| Component | Header File | Notes |
|-----------|-------------|-------|
| Types | [ctype.h](include/ctype.h) | Handle type-declaration syntax, and dealing with C types |
| Symbols | [symbol.h](include/symbol.h) | Handle symbol tables used for storing variable names and types |
| Pretty-print | [pretty_print.h](include/pretty_print.h) | Generate concise textual representations of ASTs for debugging/testing |

## Testing

ACC has unit tests - which test the components listed above individually, and regression tests - 
which test the compiler end-to-end. Both are configured to run on-commit in GitHub.

### Unit Tests

Unit tests use [CMocka](http://https://cmocka.org/) - a C test framework with support for mocking function calls.
The scanner, parser, and analysis tests take C source code as input, and verify expected behaviour, such as 
checking the generated AST, or reported errors. Calling `make test` within the top-level directory runs the unit
tests.

## Missing

Generally speaking, I've implemented language features depth-first in the front-end (how very Agile!), 
adding support for incremental subsets of the grammar in the parser and type system. Unfortunately,
this means it's missing some (many) features of the C11 grammar, which really makes it closer to C99. Let's call it C--.

### Scanner

 * `_Thread_local`, `_Noreturn`, `_Alignas`, `_Atomic`, `_Bool`, `_Complex`, `_Imaginary` type-specifiers are not implemented.
   The first four were both new additions in C11, the others were added in C99.

 * `_Alignof`, `_Generic`, and `_Static_assert` are not implemented (all were added in C11). The pre-defined identifier `__func__`
   is not implemented (added in C99).

### Parser

 * `struct` or `union` are not implemented in the parser. These will definitely be added in later.
 
 * No support for `typedef`. 

 * No support for variable length arrays (e.g., `int arr[x]`, where the size of `arr` is unknown at compile-time) 
   \- this was added in C99. In _acc_, the array-size declatation syntax is restricted to a scalar constant.
   By conrast, C89 at least permits any scalar constant expression.

 * No support for expression lists, such as `int a = 3+2, 1` (since the _expression_ rule is left-recursive,
   `a` gets the rightmost value in the list: `1`). 

 * No support for floating types (`float`/`double`). These are currently caught during the analysis, and a 
   Not-implemented yet error is raised.

 * Compound assignment operators (`+=`, `-=`, `%=`, `*=`, `/=`, `<<=`, `>>=`, `&=`, `^=`, and `|=`) are desugared
   into simple-assignment syntax. E.g., `a += 2` becomes `a = a + 2`. While this simplifies the AST, this 
   diverges from the standard (6.5.16.2): The lvalue must be evaluated only once.

### Context-sensitive Analysis

### Code generation
