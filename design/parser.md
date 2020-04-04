# Parsing

The parser in ACC receives as input a flat sequence of tokens (`include\token.h`) from the 
scanner. From this, it should determine that the program is syntactically valid and generate
the Abstract Syntax Tree (AST), or report any errors. The parser is a handcoded recursive
descent parser: rules in the grammar are implemented as mutually recursive functions.

The parser is based on the C11 grammar[1], with a handful of omissions - which might be
rectified in the future. A Yacc specification for C11 provided a template
for the parser[2], and is useful as a concise reference.

The grammar is split into three parts, each implemented seperately:
* **Expressions**: assignments, comparisions and operations (binary, unary, postfix, etc.)
* **Declarations**: including declarations, function prototypes and type names
* **Statements**: parsing high-level structure of the source input

# Links
1. C11 standard: http://www.open-std.org/jtc1/sc22/wg14/www/docs/n1570.pdf

2. C11 Yacc grammar: http://www.quut.com/c/ANSI-C-grammar-y-2011.html

