/* Recursive Descent Parser implementation (declaration)
 *
 * The recursive descent parser is implemented as a set of mutually recursive
 * functions for each rule in the grammar. This file provides functions for
 * rules in the 'declaration' set.
 *
 * Generated by generate_recursive_parser.py on 2020-03-24.
 */

#include <stddef.h>

#include "ast.h"
#include "parser.h"

AstNode* declaration(Parser* parser) {  // @TODO
  /*
   * declaration_specifiers '
   */

  return NULL;
}
AstNode* declaration_specifiers(Parser* parser) {  // @TODO
  /*
   * storage_class_specifier declaration_specifiers
   * storage_class_specifier
   * type_specifier declaration_specifiers
   * type_specifier
   * type_qualifier declaration_specifiers
   * type_qualifier
   * function_specifier declaration_specifiers
   * function_specifier
   * alignment_specifier declaration_specifiers
   * alignment_specifier
   */

  return NULL;
}
AstNode* init_declarator_list(Parser* parser) {  // @TODO
  /*
   * init_declarator
   * init_declarator_list ',' init_declarator
   */

  return NULL;
}
AstNode* init_declarator(Parser* parser) {  // @TODO
  /*
   * declarator '=' initializer
   * declarator
   */

  return NULL;
}
AstNode* storage_class_specifier(Parser* parser) {  // @TODO
  /*
   * TYPEDEF
   * EXTERN
   * STATIC
   * THREAD_LOCAL
   * AUTO
   * REGISTER
   */

  return NULL;
}
AstNode* type_specifier(Parser* parser) {  // @TODO
  /*
   * VOID
   * CHAR
   * SHORT
   * INT
   * LONG
   * FLOAT
   * DOUBLE
   * SIGNED
   * UNSIGNED
   * BOOL
   * COMPLEX
   * IMAGINARY
   * atomic_type_specifier
   * struct_or_union_specifier
   * enum_specifier
   * TYPEDEF_NAME
   */

  return NULL;
}
AstNode* struct_or_union_specifier(Parser* parser) {  // @TODO
  /*
   * struct_or_union '{' struct_declaration_list '}'
   * struct_or_union IDENTIFIER '{' struct_declaration_list '}'
   * struct_or_union IDENTIFIER
   */

  return NULL;
}
AstNode* struct_or_union(Parser* parser) {  // @TODO
  /*
   * STRUCT
   * UNION
   */

  return NULL;
}
AstNode* struct_declaration_list(Parser* parser) {  // @TODO
  /*
   * struct_declaration
   * struct_declaration_list struct_declaration
   */

  return NULL;
}
AstNode* struct_declaration(Parser* parser) {  // @TODO
  /*
   * specifier_qualifier_list '
   */

  return NULL;
}
AstNode* specifier_qualifier_list(Parser* parser) {  // @TODO
  /*
   * type_specifier specifier_qualifier_list
   * type_specifier
   * type_qualifier specifier_qualifier_list
   * type_qualifier
   */

  return NULL;
}
AstNode* struct_declarator_list(Parser* parser) {  // @TODO
  /*
   * struct_declarator
   * struct_declarator_list ',' struct_declarator
   */

  return NULL;
}
AstNode* struct_declarator(Parser* parser) {  // @TODO
  /*
   * ':' constant_expression
   * declarator ':' constant_expression
   * declarator
   */

  return NULL;
}
AstNode* enum_specifier(Parser* parser) {  // @TODO
  /*
   * ENUM '{' enumerator_list '}'
   * ENUM '{' enumerator_list ',' '}'
   * ENUM IDENTIFIER '{' enumerator_list '}'
   * ENUM IDENTIFIER '{' enumerator_list ',' '}'
   * ENUM IDENTIFIER
   */

  return NULL;
}
AstNode* enumerator_list(Parser* parser) {  // @TODO
  /*
   * enumerator
   * enumerator_list ',' enumerator
   */

  return NULL;
}
AstNode* enumerator(Parser* parser) {  // @TODO
  /*
   * enumeration_constant '=' constant_expression
   * enumeration_constant
   */

  return NULL;
}
AstNode* atomic_type_specifier(Parser* parser) {  // @TODO
  /*
   * ATOMIC '(' type_name ')'
   */

  return NULL;
}
AstNode* type_qualifier(Parser* parser) {  // @TODO
  /*
   * CONST
   * RESTRICT
   * VOLATILE
   * ATOMIC
   */

  return NULL;
}
AstNode* function_specifier(Parser* parser) {  // @TODO
  /*
   * INLINE
   * NORETURN
   */

  return NULL;
}
AstNode* alignment_specifier(Parser* parser) {  // @TODO
  /*
   * ALIGNAS '(' type_name ')'
   * ALIGNAS '(' constant_expression ')'
   */

  return NULL;
}
AstNode* declarator(Parser* parser) {  // @TODO
  /*
   * pointer direct_declarator
   * direct_declarator
   */

  return NULL;
}
AstNode* direct_declarator(Parser* parser) {  // @TODO
  /*
   * IDENTIFIER
   * '(' declarator ')'
   * direct_declarator '[' ']'
   * direct_declarator '[' '*' ']'
   * direct_declarator '[' STATIC type_qualifier_list assignment_expression ']'
   * direct_declarator '[' STATIC assignment_expression ']'
   * direct_declarator '[' type_qualifier_list '*' ']'
   * direct_declarator '[' type_qualifier_list STATIC assignment_expression ']'
   * direct_declarator '[' type_qualifier_list assignment_expression ']'
   * direct_declarator '[' type_qualifier_list ']'
   * direct_declarator '[' assignment_expression ']'
   * direct_declarator '(' parameter_type_list ')'
   * direct_declarator '(' ')'
   * direct_declarator '(' identifier_list ')'
   */

  return NULL;
}
AstNode* pointer(Parser* parser) {  // @TODO
  /*
   * '*' type_qualifier_list pointer
   * '*' type_qualifier_list
   * '*' pointer
   * '*'
   */

  return NULL;
}
AstNode* type_qualifier_list(Parser* parser) {  // @TODO
  /*
   * type_qualifier
   * type_qualifier_list type_qualifier
   */

  return NULL;
}
AstNode* parameter_type_list(Parser* parser) {  // @TODO
  /*
   * parameter_list ',' ELLIPSIS
   * parameter_list
   */

  return NULL;
}
AstNode* parameter_list(Parser* parser) {  // @TODO
  /*
   * parameter_declaration
   * parameter_list ',' parameter_declaration
   */

  return NULL;
}
AstNode* parameter_declaration(Parser* parser) {  // @TODO
  /*
   * declaration_specifiers declarator
   * declaration_specifiers abstract_declarator
   * declaration_specifiers
   */

  return NULL;
}
AstNode* identifier_list(Parser* parser) {  // @TODO
  /*
   * IDENTIFIER
   * identifier_list ',' IDENTIFIER
   */

  return NULL;
}
AstNode* type_name(Parser* parser) {  // @TODO
  /*
   * specifier_qualifier_list abstract_declarator
   * specifier_qualifier_list
   */

  return NULL;
}
AstNode* abstract_declarator(Parser* parser) {  // @TODO
  /*
   * pointer direct_abstract_declarator
   * pointer
   * direct_abstract_declarator
   */

  return NULL;
}
AstNode* direct_abstract_declarator(Parser* parser) {  // @TODO
  /*
	 * '(' abstract_declarator ')'
	 * '[' ']'
	 * '[' constant_expression ']'
	 * direct_abstract_declarator '[' ']'
	 * direct_abstract_declarator '[' constant_expression ']'
	 * '(' ')'
	 * '(' parameter_type_list ')'
	 * direct_abstract_declarator '(' ')'
	 * direct_abstract_declarator '(' parameter_type_list ')'
   */

  return NULL;
}
AstNode* initializer(Parser* parser) {  // @TODO
  /*
   * '{' initializer_list '}'
   * '{' initializer_list ',' '}'
   * assignment_expression
   */

  return NULL;
}
AstNode* initializer_list(Parser* parser) {  // @TODO
  /*
   * designation initializer
   * initializer
   * initializer_list ',' designation initializer
   * initializer_list ',' initializer
   */

  return NULL;
}
AstNode* designation(Parser* parser) {  // @TODO
  /*
   * designator_list '='
   */

  return NULL;
}
AstNode* designator_list(Parser* parser) {  // @TODO
  /*
   * designator
   * designator_list designator
   */

  return NULL;
}
AstNode* designator(Parser* parser) {  // @TODO
  /*
   * '[' constant_expression ']'
   * '.' IDENTIFIER
   */

  return NULL;
}
AstNode* static_assert_declaration(Parser* parser) {  // @TODO
  /*
   * STATIC_ASSERT '(' constant_expression ',' STRING_LITERAL ')' '
   */

  return NULL;
}
AstNode* translation_unit(Parser* parser) {  // @TODO
  /*
   * external_declaration
   * translation_unit external_declaration
   */

  return NULL;
}
AstNode* external_declaration(Parser* parser) {  // @TODO
  /*
   * function_definition
   * declaration
   */

  return NULL;
}
AstNode* function_definition(Parser* parser) {  // @TODO
  /*
   * declaration_specifiers declarator declaration_list compound_statement
   * declaration_specifiers declarator compound_statement
   */

  return NULL;
}
AstNode* declaration_list(Parser* parser) {  // @TODO
  /*
   * declaration
   * declaration_list declaration
   */

  return NULL;
}
