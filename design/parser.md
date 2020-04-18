# Parsing

The parser in ACC receives as input a flat sequence of tokens (`include/token.h`) from the 
scanner. From this, it should determine that the program is syntactically valid and generate
the Abstract Syntax Tree (AST), or report any errors. The parser is a handcoded recursive
descent parser: rules in the grammar are implemented as mutually recursive functions.

The parser is based on the C11 grammar[1], with a handful of omissions - which might be
rectified in the future. A Yacc specification for C11 provided a template
for the parser[2], and is useful as a concise reference.

The grammar is split into three parts, each implemented seperately:
* **Expressions**: assignments, comparisions and operations (binary, unary, postfix, etc.),
  implemented in `source/parser_expression.c`.
* **Declarations**: including declarations, function prototypes and type names,
  implemented in `source/parser_declaration.c`.
* **Statements**: parsing high-level structure of the source input, implemented in
  `source/parser_statement.c`.

The entry-point in the grammar is `translation_unit`, which derives a flat sequence of
declarations:
```
translation_unit: (function_declaration | declaration)*
```

`function_declarations` subsequently derive `compound_statements`, which include:
* `expression_statement`, including assignments and function-calls.
* `declaration` - although not function-declarations of course.
* `iteration_statement` - including `while` and `for`.

For some rules - provided the semantics and set of derivations are unchanged,
the parser adapts the grammar to suit a recursive descent implementation. These
changes are discussed in the sections below.

## Expressions

## Declarations

Declarations includes variables and functions. The grammar treats both seperately,
which makes it easier to bake function-specific type rules (`inline` and `noreturn`), and 
limitations on where function definitions can be placed into the grammar. Since the two
_FIRST_ sets are not disjoint, `parser_declaration.c` handles both cases in the `Parser_declaration()`
function:
```c
  if (decl->type->type == TYPE_FUNCTION && peek()->type == LEFT_BRACE) {
    decl->body = Parser_compound_statement(parser);
  } else {
    consume(SEMICOLON);
  }
```

## Statements




# Links
1. C11 standard: http://www.open-std.org/jtc1/sc22/wg14/www/docs/n1570.pdf

2. C11 Yacc grammar: http://www.quut.com/c/ANSI-C-grammar-y-2011.html

