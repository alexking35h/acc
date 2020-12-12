/* Recursive Descent Parser implementation (declaration)
 *
 * The recursive descent parser is implemented as a set of mutually recursive
 * functions for each rule in the grammar. This file provides functions for
 * rules in the 'declaration' set.
 *
 * Generated by generate_recursive_parser.py on 2020-03-24.
 */

#include <setjmp.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "ast.h"
#include "ctype.h"
#include "parser.h"

#define DECL(...) Ast_create_decl_node((DeclAstNode){__VA_ARGS__})

#define match(...) Parser_match_token(parser, (TokenType[]){__VA_ARGS__, NAT})
#define consume(t) Parser_consume_token(parser, t)
#define peek() Parser_peek_token(parser)
#define advance() Parser_advance_token(parser)
#define sync(...) Parser_sync_token(parser, (TokenType[]){__VA_ARGS__, NAT})

#define static

static CType *declaration_specifiers(Parser *parser);                  // @TODO
static DeclAstNode *init_declarator_list(Parser *parser, CType *type); // @TODO
static DeclAstNode *init_declarator(Parser *parser, CType *type);      // @TODO
static DeclAstNode *storage_class_specifier(Parser *parser);           // @TODO
static DeclAstNode *type_specifier(Parser *parser);                    // @TODO
static DeclAstNode *struct_or_union_specifier(Parser *parser);         // @TODO
static DeclAstNode *struct_or_union(Parser *parser);                   // @TODO
static DeclAstNode *struct_declaration_list(Parser *parser);           // @TODO
static DeclAstNode *struct_declaration(Parser *parser);                // @TODO
static DeclAstNode *specifier_qualifier_list(Parser *parser);          // @TODO
static DeclAstNode *struct_declarator_list(Parser *parser);            // @TODO
static DeclAstNode *struct_declarator(Parser *parser);                 // @TODO
static DeclAstNode *enum_specifier(Parser *parser);                    // @TODO
static DeclAstNode *enumerator_list(Parser *parser);                   // @TODO
static DeclAstNode *enumerator(Parser *parser);                        // @TODO
static DeclAstNode *atomic_type_specifier(Parser *parser);             // @TODO
static DeclAstNode *type_qualifier(Parser *parser);                    // @TODO
static DeclAstNode *function_specifier(Parser *parser);                // @TODO
static DeclAstNode *alignment_specifier(Parser *parser);               // @TODO
static DeclAstNode *declarator(Parser *parser, CType *ctype);          // @TODO
static CType *direct_declarator_end(Parser *, CType *);
static DeclAstNode *direct_declarator(Parser *parser, CType *ctype); // @TODO
static DeclAstNode *pointer(Parser *parser);                         // @TODO
static DeclAstNode *type_qualifier_list(Parser *parser);             // @TODO
static ParameterListItem *parameter_type_list(Parser *parser);       // @TODO
static ParameterListItem *parameter_list(Parser *parser);            // @TODO
static ParameterListItem *parameter_declaration(Parser *parser);     // @TODO
static DeclAstNode *identifier_list(Parser *parser);                 // @TODO
static DeclAstNode *abstract_declarator(Parser *parser);             // @TODO
static DeclAstNode *direct_abstract_declarator(Parser *parser);      // @TODO
static ExprAstNode *initializer(Parser *parser);                     // @TODO
static DeclAstNode *initializer_list(Parser *parser);                // @TODO
static DeclAstNode *designation(Parser *parser);                     // @TODO
static DeclAstNode *designator_list(Parser *parser);                 // @TODO
static DeclAstNode *designator(Parser *parser);                      // @TODO
static DeclAstNode *static_assert_declaration(Parser *parser);       // @TODO
static DeclAstNode *translation_unit(Parser *parser);                // @TODO
static DeclAstNode *external_declaration(Parser *parser);            // @TODO
static DeclAstNode *function_definition(Parser *parser);             // @TODO
static DeclAstNode *declaration_list(Parser *parser);                // @TODO

DeclAstNode *Parser_declaration(Parser *parser)
{
    /*
     * declaration_specifiers init_declarator_list ';'
     * declaration_specifiers init_declarator_list compound_statement
     */
    Position pos = peek()->pos;

    CType *type = declaration_specifiers(parser);

    DeclAstNode *decl = init_declarator_list(parser, type);

    // If the ctype is a function, check for a compound statement after
    if (decl->type->type == TYPE_FUNCTION && peek()->type == LEFT_BRACE)
    {
        decl->body = Parser_compound_statement(parser);
    }
    else
    {
        consume(SEMICOLON);
    }

    // Check that the declarator is 'concrete', i.e., that an identifier
    // has been provided.
    if (!decl->identifier)
    {
        Error_report_error(parser->error_reporter, PARSER, pos,
                           "Missing identifier in declaration");
        THROW_ERROR(parser);
    }

    return decl;
}
static CType *declaration_specifiers(Parser *parser)
{
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
    CType *type = calloc(1, sizeof(CType));

    Position pos = peek()->pos;

    char *err = NULL;
    while (true)
    {
        TokenType token_type = peek()->type;

        switch (token_type)
        {
        // Type specifiers
        // int, char, void, short, long, signed, unsigned
        case INT:
            ctype_set_basic_specifier(type, TYPE_INT);
            break;
        case CHAR:
            ctype_set_basic_specifier(type, TYPE_CHAR);
            break;
        case VOID:
            ctype_set_basic_specifier(type, TYPE_VOID);
            break;

        case SHORT:
            ctype_set_basic_specifier(type, TYPE_SHORT);
            break;

        case SIGNED:
            ctype_set_basic_specifier(type, TYPE_SIGNED);
            break;
        case UNSIGNED:
            ctype_set_basic_specifier(type, TYPE_UNSIGNED);
            break;

        // Type qualifiers
        // const, volatile
        case CONST:
            ctype_set_qualifier(type, TYPE_CONST);
            break;
        case VOLATILE:
            ctype_set_qualifier(type, TYPE_VOLATILE);
            break;

        // Storage-specifiers
        // extern, auto, static, register
        case EXTERN:
            ctype_set_storage_specifier(type, TYPE_EXTERN);
            break;
        case AUTO:
            ctype_set_storage_specifier(type, TYPE_AUTO);
            break;
        case STATIC:
            ctype_set_storage_specifier(type, TYPE_STATIC);
            break;
        case REGISTER:
            ctype_set_storage_specifier(type, TYPE_REGISTER);
            break;

        default:
            goto end;
        }

        advance();
    }
end:
    ctype_finalise(type, &err);
    if (err)
    {
        Error_report_error(parser->error_reporter, PARSER, pos, err);
        THROW_ERROR(parser);
    }
    return type;
}
static DeclAstNode *init_declarator_list(Parser *parser, CType *type)
{
    /* init_declarator ',' | init_declarator */
    DeclAstNode *head = init_declarator(parser, type);
    DeclAstNode **curr = &(head->next);

    while (match(COMMA))
    {
        *curr = init_declarator(parser, type);
        curr = &((*curr)->next);
    }

    return head;
}
static DeclAstNode *init_declarator(Parser *parser, CType *type)
{
    /* declarator '=' initializer | declarator */
    Position pos = peek()->pos;
    DeclAstNode *decl = declarator(parser, type);

    char *err = NULL;
    ctype_finalise(decl->type, &err);
    if (err)
    {
        Error_report_error(parser->error_reporter, PARSER, pos, err);
        THROW_ERROR(parser);
    }

    if (match(EQUAL))
        decl->initializer = initializer(parser);

    return decl;
}

static DeclAstNode *struct_or_union_specifier(Parser *parser)
{ // @TODO
    /*
     * struct_or_union '{' struct_declaration_list '}'
     * struct_or_union IDENTIFIER '{' struct_declaration_list '}'
     * struct_or_union IDENTIFIER
     */

    return NULL;
}
static DeclAstNode *struct_or_union(Parser *parser)
{ // @TODO
    /*
     * STRUCT
     * UNION
     */

    return NULL;
}
static DeclAstNode *struct_declaration_list(Parser *parser)
{ // @TODO
    /*
     * struct_declaration
     * struct_declaration_list struct_declaration
     */

    return NULL;
}
static DeclAstNode *struct_declaration(Parser *parser)
{ // @TODO
    /*
     * specifier_qualifier_list '
     */

    return NULL;
}
static DeclAstNode *specifier_qualifier_list(Parser *parser)
{ // @TODO
    /*
     * type_specifier specifier_qualifier_list
     * type_specifier
     * type_qualifier specifier_qualifier_list
     * type_qualifier
     */

    return NULL;
}
static DeclAstNode *struct_declarator_list(Parser *parser)
{ // @TODO
    /*
     * struct_declarator
     * struct_declarator_list ',' struct_declarator
     */

    return NULL;
}
static DeclAstNode *struct_declarator(Parser *parser)
{ // @TODO
    /*
     * ':' constant_expression
     * declarator ':' constant_expression
     * declarator
     */

    return NULL;
}
static DeclAstNode *enum_specifier(Parser *parser)
{ // @TODO
    /*
     * ENUM '{' enumerator_list '}'
     * ENUM '{' enumerator_list ',' '}'
     * ENUM IDENTIFIER '{' enumerator_list '}'
     * ENUM IDENTIFIER '{' enumerator_list ',' '}'
     * ENUM IDENTIFIER
     */

    return NULL;
}
static DeclAstNode *enumerator_list(Parser *parser)
{ // @TODO
    /*
     * enumerator
     * enumerator_list ',' enumerator
     */

    return NULL;
}
static DeclAstNode *enumerator(Parser *parser)
{ // @TODO
    /*
     * enumeration_constant '=' constant_expression
     * enumeration_constant
     */

    return NULL;
}
static DeclAstNode *atomic_type_specifier(Parser *parser)
{ // @TODO
    /*
     * ATOMIC '(' type_name ')'
     */

    return NULL;
}
static DeclAstNode *function_specifier(Parser *parser)
{ // @TODO
    /*
     * INLINE
     * NORETURN
     */

    return NULL;
}
static DeclAstNode *alignment_specifier(Parser *parser)
{ // @TODO
    /*
     * ALIGNAS '(' type_name ')'
     * ALIGNAS '(' constant_expression ')'
     */

    return NULL;
}
static DeclAstNode *declarator(Parser *parser, CType *ctype)
{
    /* '*' direct_declarator | direct_declarator */
    while (match(STAR))
    {
        CType *pointer = calloc(1, sizeof(CType));
        pointer->type = TYPE_POINTER;
        ctype_set_derived(pointer, ctype);
        ctype = pointer;
    }

    return direct_declarator(parser, ctype);
}
static DeclAstNode *direct_declarator(Parser *parser, CType *ctype)
{
    /*
     * IDENTIFIER direct_declarator_end
     * '(' declarator ')' direct_declarator_end
     */
    Token *tok;
    DeclAstNode *decl_node;
    if ((tok = match(IDENTIFIER)))
    {
        ctype = direct_declarator_end(parser, ctype);
        return DECL(CONCRETE, .pos = tok->pos, .identifier = tok, .type = ctype);
    }
    else if (match(LEFT_PAREN))
    {
        CType tmp_type;
        decl_node = declarator(parser, &tmp_type);
        consume(RIGHT_PAREN);

        CType *parent_type = tmp_type.parent_type;
        CType *child_type = direct_declarator_end(parser, ctype);

        ctype_set_derived(parent_type, child_type);

        return decl_node;
    }

    // Abstract declarator!
    return DECL(.decl_type = ABSTRACT, .type = direct_declarator_end(parser, ctype));
}

static CType *direct_declarator_end(Parser *parser, CType *ctype)
{
    /*
     * direct_declarator_end '[' constant_expression ? ']'
     * direct_declarator_end '(' parameter_type_list ? ')'
     */
    if (match(LEFT_SQUARE))
    {
        CType *next = calloc(1, sizeof(CType));
        next->type = TYPE_ARRAY;
        next->derived.array_size = consume(CONSTANT)->literal.const_value;
        consume(RIGHT_SQUARE);

        ctype_set_derived(next, direct_declarator_end(parser, ctype));
        return next;
    }
    else if (match(LEFT_PAREN))
    {
        CType *next = calloc(1, sizeof(CType));
        next->type = TYPE_FUNCTION;

        if (match(RIGHT_PAREN))
        {
            next->derived.params = NULL;
        }
        else
        {
            next->derived.params = parameter_type_list(parser);
            consume(RIGHT_PAREN);
        }
        ctype_set_derived(next, direct_declarator_end(parser, ctype));
        return next;
    }
    else
        return ctype;
}
static DeclAstNode *type_qualifier_list(Parser *parser)
{ // @TODO
    /*
     * type_qualifier
     * type_qualifier_list type_qualifier
     */

    return NULL;
}
static ParameterListItem *parameter_type_list(Parser *parser)
{
    /* parameter_list ',' | parameter_list */
    return parameter_list(parser);

    ParameterListItem *head;
    ParameterListItem **curr = &head;

    do
    {
        *curr = calloc(1, sizeof(ParameterListItem));

        (*curr)->type = declaration_specifiers(parser);

        DeclAstNode *tmp_node = declarator(parser, (*curr)->type);
        (*curr)->type = tmp_node->type;

        if (tmp_node->decl_type == CONCRETE)
        {
            (*curr)->name = tmp_node->identifier;
        }
        else
        {
            (*curr)->name = NULL;
        }

        curr = &((*curr)->next);

    } while (match(COMMA));

    return head;
}
static ParameterListItem *parameter_list(Parser *parser)
{
    /* parameter_list ',' parameter_declaration | parameter_declaration */
    if (CATCH_ERROR(parser))
    {
        // Error occurred parsing 'parameter_declaration'
        sync(COMMA, RIGHT_PAREN, END_OF_FILE);

        return match(COMMA) ? parameter_list(parser) : NULL;
    }
    ParameterListItem *param = parameter_declaration(parser);

    if (match(COMMA))
    {
        param->next = parameter_list(parser);
    }
    else
    {
        param->next = NULL;
    }
    return param;
}
static ParameterListItem *parameter_declaration(Parser *parser)
{ // @TODO
    /*
     * declaration_specifiers declarator
     * declaration_specifiers abstract_declarator
     * declaration_specifiers
     */
    ParameterListItem *param = calloc(1, sizeof(ParameterListItem));

    CType *primitive_type = declaration_specifiers(parser);
    DeclAstNode *tmp_node = declarator(parser, primitive_type);

    param->type = tmp_node->type;

    if (tmp_node->decl_type == CONCRETE)
    {
        param->name = tmp_node->identifier;
    }
    else
    {
        param->name = NULL;
    }

    return param;
}

CType *Parser_type_name(Parser *parser)
{ // @TODO
    /*
     * specifier_qualifier_list abstract_declarator
     * specifier_qualifier_list
     */
    Position position = peek()->pos;
    DeclAstNode *decl = declarator(parser, declaration_specifiers(parser));

    // Check that it is an abstract declarator (identifier has
    // been omitted)
    if (decl->decl_type == CONCRETE)
    {
        Error_report_error(parser->error_reporter, PARSER, position,
                           "Type names must not have an identifier");
        THROW_ERROR(parser);
    }
    return decl->type;
}
static ExprAstNode *initializer(Parser *parser)
{ // @TODO
    /*
     * '{' initializer_list '}'
     * '{' initializer_list ',' '}'
     * assignment_expression
     */
    return Parser_expression(parser);
}
static DeclAstNode *initializer_list(Parser *parser)
{ // @TODO
    /*
     * designation initializer
     * initializer
     * initializer_list ',' designation initializer
     * initializer_list ',' initializer
     */

    return NULL;
}
static DeclAstNode *designation(Parser *parser)
{ // @TODO
    /*
     * designator_list '='
     */

    return NULL;
}
static DeclAstNode *designator_list(Parser *parser)
{ // @TODO
    /*
     * designator
     * designator_list designator
     */

    return NULL;
}
static DeclAstNode *designator(Parser *parser)
{ // @TODO
    /*
     * '[' constant_expression ']'
     * '.' IDENTIFIER
     */

    return NULL;
}
static DeclAstNode *static_assert_declaration(Parser *parser)
{ // @TODO
    /*
     * STATIC_ASSERT '(' constant_expression ',' STRING_LITERAL ')' '
     */

    return NULL;
}
static DeclAstNode *Parser_translation_unit(Parser *parser)
{
    /* translation_unit declaration | declaration */
    DeclAstNode *head = NULL, **curr = &head;
    while ((peek())->type != END_OF_FILE)
    {
        if (CATCH_ERROR(parser))
        {
            // Error occurred parsing the following declaration.
            // synchronise on the next semi-colon.
            sync(SEMICOLON, END_OF_FILE);

            if (peek()->type == SEMICOLON)
                advance();
            continue;
        }

        *curr = Parser_declaration(parser);
        curr = &((*curr)->next);
    }
    return head;
}