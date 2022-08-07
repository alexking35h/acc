/*
 * Recursive Descent Parser implementation (expression)
 *
 * The recursive descent parser is implemented as a set of mutually recursive
 * functions for each rule in the grammar. This file provides functions for
 * rules in the 'expression' set.
 */

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "ast.h"
#include "parser.h"
#include "token.h"
#include "util.h"

typedef struct Parser_t
{
    // Scanner and error instances.
    Scanner *scanner;
    ErrorReporter *error_reporter;

    // Next unread token.
    Token *next_token[2];
    int next_token_index;

    jmp_buf panic_jmp;

} Parser;

/*
 * Rudimentary try/catch statements using setjmp and longjmp. E.g.:
 *  if(CATCH_ERROR(parser)) {
 *    // Error occurred!
 *  } else {
 *    //...
 *    THROW_ERROR(parser)
 *  }
 */
#define CATCH_ERROR(parser) (setjmp(parser->panic_jmp) != 0)
#define THROW_ERROR(parser) longjmp(parser->panic_jmp, 1)

#define EXPR_BINARY(p, ...)                                                              \
    Ast_create_expr_node((ExprAstNode){BINARY, p, .binary = {__VA_ARGS__}})
#define EXPR_UNARY(p, ...)                                                               \
    Ast_create_expr_node((ExprAstNode){UNARY, p, .unary = {__VA_ARGS__}})
#define EXPR_PRIMARY(p, ...)                                                             \
    Ast_create_expr_node((ExprAstNode){PRIMARY, p, .primary = {__VA_ARGS__}})
#define EXPR_POSTFIX(p, ...)                                                             \
    Ast_create_expr_node((ExprAstNode){POSTFIX, p, .postfix = {__VA_ARGS__}})
#define EXPR_CAST(p, ...)                                                                \
    Ast_create_expr_node((ExprAstNode){CAST, p, .cast = {__VA_ARGS__}})
#define EXPR_TERTIARY(p, ...)                                                            \
    Ast_create_expr_node((ExprAstNode){TERTIARY, p, .tertiary = {__VA_ARGS__}})
#define EXPR_ASSIGN(p, ...)                                                              \
    Ast_create_expr_node((ExprAstNode){ASSIGN, p, .assign = {__VA_ARGS__}})
#define STMT_EXPR(p, ...)                                                                \
    Ast_create_stmt_node((StmtAstNode){.type = EXPR, p, .expr = {__VA_ARGS__}})
#define STMT_DECL(p, ...)                                                                \
    Ast_create_stmt_node((StmtAstNode){.type = DECL, p, .decl = {__VA_ARGS__}})
#define STMT_BLOCK(p, ...)                                                               \
    Ast_create_stmt_node((StmtAstNode){.type = BLOCK, p, .block = {__VA_ARGS__}})
#define STMT_WHILE(p, ...)                                                               \
    Ast_create_stmt_node(                                                                \
        (StmtAstNode){.type = WHILE_LOOP, p, .while_loop = {__VA_ARGS__}})
#define STMT_RETURN(p, ...)                                                              \
    Ast_create_stmt_node(                                                                \
        (StmtAstNode){.type = RETURN_JUMP, p, .return_jump = {__VA_ARGS__}})
#define STMT_IF(p, ...)                                                                  \
    Ast_create_stmt_node(                                                                \
        (StmtAstNode){.type = IF_STATEMENT, p, .if_statement = {__VA_ARGS__}})
#define DECL(...) Ast_create_decl_node((DeclAstNode){__VA_ARGS__})

#define consume(t) consume_token(parser, t)
#define peek() peek_token(parser)
#define peek_next() peek_next_token(parser)
#define match(...) match_token(parser, (TokenType[]){__VA_ARGS__, NAT})
#define advance(...) advance_token(parser)
#define sync(...) sync_token(parser, (TokenType[]){__VA_ARGS__, NAT})

// If the next token matches, advance the token stream, and return the token.
// Return NULL otherwise.
static Token *match_token(Parser *, TokenType *);

// If the next token does not match, report an error and return false.
static Token *consume_token(Parser *, TokenType);

// Check out the next token, and the next+1 token
static inline Token *peek_token(Parser * parser) {
    return parser->next_token[parser->next_token_index];
}
static inline Token *peek_next_token(Parser * parser) {
    int ind = (parser->next_token_index + 1) % 2;
    return parser->next_token[ind];
}

// Advance the parser to the next token
static void advance_token(Parser * parser) {
    parser->next_token[parser->next_token_index] = Scanner_get_next(parser->scanner);
    parser->next_token_index = (parser->next_token_index + 1) % 2;
}

// Synchronise the token stream input on any token in set types.
// The array types must end with NAT. (E.g., {SEMICOLON, EOF, NAT})
static inline void sync_token(Parser *parser, TokenType types[])
{
    while (1)
    {
        for (TokenType *t = &types[0]; *t != NAT; t++)
        {
            if (peek_token(parser)->type == *t)
                return;
        }
        advance_token(parser);
    }
}

// Required forward declarations for statements
static StmtAstNode *statement(Parser *parser);

// Required forward definitions for declarations
static DeclAstNode *declarator(Parser *parser, CType *ctype);
static CType *type_name(Parser *parser);

static ExprAstNode *desugar_assign(Parser *parser, ExprAstNode *expr, BinaryExprOp op,
                                   ExprAstNode *operand)
{
    /* A bunch of the C syntax is treated as syntactic sugar, to make the AST more
     * homogeneous and (hopefully) make it easier to implement later parts of the
     * compiler. This includes ++ and -- unary operators, and assignment
     * operators (+=, -=, /=, etc.)
     */
    ExprAstNode *op_expr =
        EXPR_BINARY(expr->pos, .left = expr, .op = op, .right = operand);
    return EXPR_ASSIGN(expr->pos, .left = expr, .right = op_expr);
}

static ExprAstNode *desugar_array(Parser *parser, ExprAstNode *base, ExprAstNode *index)
{
    ExprAstNode *binary_op =
        EXPR_BINARY(base->pos, .left = base, .op = BINARY_ADD, .right = index);
    ExprAstNode *ptr_op =
        EXPR_UNARY(base->pos, .op = UNARY_DEREFERENCE, .right = binary_op);
    return ptr_op;
}

static ExprAstNode *primary_expression(Parser *parser)
{
    Token *next = match(IDENTIFIER, CONSTANT, LEFT_PAREN);

    if (!next)
    {
        char *err_str =
            STR_CONCAT("Expected expression, got '", Token_str(peek()->type), "'");
        Error_report_error(parser->error_reporter, PARSER, peek()->pos, err_str);
        THROW_ERROR(parser);
    }

    if (next->type == IDENTIFIER)
    {
        return EXPR_PRIMARY(next->pos, .identifier = next);
    }
    else if (next->type == CONSTANT)
    {
        return EXPR_PRIMARY(next->pos, .constant = next);
    }
    else if (next->type == LEFT_PAREN)
    {
        ExprAstNode *expr = Parser_expression(parser);
        consume(RIGHT_PAREN);
        return expr;
    }
    return NULL;
}
static ArgumentListItem *argument_expression_list(Parser *parser)
{
    if (peek()->type == RIGHT_PAREN)
        return NULL;

    ArgumentListItem *arg = calloc(1, sizeof(ArgumentListItem));
    arg->argument = Parser_expression(parser);
    arg->next = match(COMMA) ? argument_expression_list(parser) : NULL;
    return arg;
}

static ExprAstNode *postfix_expression(Parser *parser)
{
    ExprAstNode *expr = primary_expression(parser);
    Token *token;

    while ((token = match(LEFT_SQUARE, LEFT_PAREN, INC_OP, DEC_OP)))
    {
        if (token->type == LEFT_SQUARE)
        {
            ExprAstNode *index = Parser_expression(parser);
            consume(RIGHT_SQUARE);
            expr = desugar_array(parser, expr, index);
        }
        else if (token->type == LEFT_PAREN)
        {
            // Function call.
            expr = EXPR_POSTFIX(expr->pos, .left = expr, .op = POSTFIX_CALL,
                                .args = argument_expression_list(parser));
            consume(RIGHT_PAREN);
        }
        else
        {
            expr = EXPR_POSTFIX(
                token->pos, .op = token->type == INC_OP ? POSTFIX_INC_OP : POSTFIX_DEC_OP,
                .left = expr);
        }
    }
    return expr;
}
static ExprAstNode *unary_expression(Parser *parser)
{
    Token *token =
        match(AMPERSAND, STAR, PLUS, MINUS, TILDE, BANG, SIZEOF, INC_OP, DEC_OP);
    if (token)
    {
        UnaryExprOp op;
        switch (token->type)
        {
        case AMPERSAND:
            op = UNARY_ADDRESS_OF;
            break;
        case STAR:
            op = UNARY_DEREFERENCE;
            break;
        case PLUS:
            op = UNARY_PLUS;
            break;
        case MINUS:
            op = UNARY_MINUS;
            break;
        case TILDE:
            op = UNARY_BITWISE_NOT;
            break;
        case BANG:
            op = UNARY_LOGICAL_NOT;
            break;
        case SIZEOF:
            op = UNARY_SIZEOF;
            break;
        case INC_OP:
            op = UNARY_INC_OP;
            break;
        case DEC_OP:
            op = UNARY_DEC_OP;
            break;
        }
        return EXPR_UNARY(token->pos, .op = op, .right = unary_expression(parser));
    }
    return postfix_expression(parser);
}

static ExprAstNode *cast_expression(Parser *parser)
{
    if (peek()->type != LEFT_PAREN)
        return unary_expression(parser);

    // We may be looking at a cast expression, such as:
    // (int)var;
    // We can't know, without looking at the next+1 token, to see
    // if it's a type-specifier/qualifier/storage-specifier.
    switch (peek_next()->type)
    {
    case VOID:
    case CHAR:
    case SHORT:
    case INT:
    case SIGNED:
    case UNSIGNED:
        break;
    default:
        return unary_expression(parser);
    }

    // Cast expression.
    consume(LEFT_PAREN);
    CType *type = type_name(parser);
    consume(RIGHT_PAREN);
    return EXPR_CAST(peek()->pos, .to = type, .from = NULL,
                     .right = cast_expression(parser));
}
static ExprAstNode *multiplicative_expression(Parser *parser)
{
    ExprAstNode *expr = cast_expression(parser);
    Token *operator;

    while ((operator= match(STAR, SLASH, PERCENT)))
    {
        ExprAstNode *right = cast_expression(parser);

        if (operator->type == STAR)
        {
            expr = EXPR_BINARY(operator->pos, .left = expr, .op = BINARY_MUL,
                               .right = right);
        }
        else if (operator->type == SLASH)
        {
            expr = EXPR_BINARY(operator->pos, .left = expr, .op = BINARY_DIV,
                               .right = right);
        }
        else
        {
            expr = EXPR_BINARY(operator->pos, .left = expr, .op = BINARY_MOD,
                               .right = right);
        }
    }
    return expr;
}
static ExprAstNode *additive_expression(Parser *parser)
{
    ExprAstNode *expr = multiplicative_expression(parser);
    Token *operator;

    while ((operator= match(PLUS, MINUS)))
    {
        ExprAstNode *right = multiplicative_expression(parser);

        if (operator->type == PLUS)
        {
            expr = EXPR_BINARY(operator->pos, .left = expr, .op = BINARY_ADD,
                               .right = right);
        }
        else
        {
            expr = EXPR_BINARY(operator->pos, .left = expr, .op = BINARY_SUB,
                               .right = right);
        }
    }
    return expr;
}
static ExprAstNode *shift_expression(Parser *parser)
{
    ExprAstNode *expr = additive_expression(parser);
    Token *operator;

    while ((operator= match(LEFT_OP, RIGHT_OP)))
    {
        ExprAstNode *right = additive_expression(parser);
        if (operator->type == LEFT_OP)
        {
            expr = EXPR_BINARY(operator->pos, .left = expr, .op = BINARY_SLL,
                               .right = right);
        }
        else
        {
            expr = EXPR_BINARY(operator->pos, .left = expr, .op = BINARY_SLR,
                               .right = right);
        }
    }
    return expr;
}
static ExprAstNode *relational_expression(Parser *parser)
{
    ExprAstNode *expr = shift_expression(parser);
    Token *operator;

    while ((operator= match(LESS_THAN, GREATER_THAN, LE_OP, GE_OP)))
    {
        ExprAstNode *right = shift_expression(parser);
        if (operator->type == LESS_THAN)
        {
            expr =
                EXPR_BINARY(operator->pos, .left = expr, .op = BINARY_LT, .right = right);
        }
        else if (operator->type == GREATER_THAN)
        {
            expr =
                EXPR_BINARY(operator->pos, .left = expr, .op = BINARY_GT, .right = right);
        }
        else if (operator->type == LE_OP)
        {
            expr =
                EXPR_BINARY(operator->pos, .left = expr, .op = BINARY_LE, .right = right);
        }
        else
        {
            expr =
                EXPR_BINARY(operator->pos, .left = expr, .op = BINARY_GE, .right = right);
        }
    }
    return expr;
}
static ExprAstNode *equality_expression(Parser *parser)
{
    ExprAstNode *expr = relational_expression(parser);
    Token *operator;

    while ((operator= match(EQ_OP, NE_OP)))
    {
        ExprAstNode *right = relational_expression(parser);
        if (operator->type == EQ_OP)
        {
            expr =
                EXPR_BINARY(operator->pos, .left = expr, .op = BINARY_EQ, .right = right);
        }
        else
        {
            expr =
                EXPR_BINARY(operator->pos, .left = expr, .op = BINARY_NE, .right = right);
        }
    }
    return expr;
}
static ExprAstNode *and_expression(Parser *parser)
{
    ExprAstNode *expr = equality_expression(parser);
    Token *operator;

    while ((operator= match(AMPERSAND)))
    {
        ExprAstNode *right = equality_expression(parser);
        expr = EXPR_BINARY(operator->pos, .op = BINARY_AND, .left = expr, .right = right);
    }
    return expr;
}
static ExprAstNode *exclusive_or_expression(Parser *parser)
{
    ExprAstNode *expr = and_expression(parser);
    Token *operator;

    while ((operator= match(CARET)))
    {
        ExprAstNode *right = and_expression(parser);
        expr = EXPR_BINARY(operator->pos, .op = BINARY_XOR, .left = expr, .right = right);
    }
    return expr;
}
static ExprAstNode *inclusive_or_expression(Parser *parser)
{
    ExprAstNode *expr = exclusive_or_expression(parser);
    Token *operator;

    while ((operator= match(BAR)))
    {
        ExprAstNode *right = exclusive_or_expression(parser);
        expr = EXPR_BINARY(operator->pos, .op = BINARY_OR, .left = expr, .right = right);
    }
    return expr;
}
static ExprAstNode *logical_and_expression(Parser *parser)
{
    ExprAstNode *expr = inclusive_or_expression(parser);
    Token *operator;

    while ((operator= match(AND_OP)))
    {
        ExprAstNode *right = inclusive_or_expression(parser);
        expr =
            EXPR_BINARY(operator->pos, .op = BINARY_AND_OP, .left = expr, .right = right);
    }
    return expr;
}
static ExprAstNode *logical_or_expression(Parser *parser)
{
    ExprAstNode *expr = logical_and_expression(parser);
    Token *operator;

    while ((operator= match(OR_OP)))
    {
        ExprAstNode *right = logical_and_expression(parser);
        expr =
            EXPR_BINARY(operator->pos, .op = BINARY_OR_OP, .left = expr, .right = right);
    }
    return expr;
}

static ExprAstNode *conditional_expression(Parser *parser)
{
    ExprAstNode *expr = logical_or_expression(parser);
    Token *question = match(QUESTION);
    if (!question)
        return expr;

    ExprAstNode *expr_true = Parser_expression(parser);
    consume(COLON);
    ExprAstNode *expr_false = conditional_expression(parser);

    return EXPR_TERTIARY(question->pos, .condition_expr = expr, .expr_true = expr_true,
                         .expr_false = expr_false);
}

ExprAstNode *Parser_assignment_expression(Parser *parser)
{
    // The FIRST sets for the grammar rules 'conditional_expression' and
    // 'unary_expression' are not disjoint. To sidestep this,
    // 'Parser_assignment_expression is parsed as: : conditional_expression |
    // conditional_expression assignment_operator conditional_expression.
    //
    // Later we'll come back to make sure the lvalue is valid.
    ExprAstNode *expr = conditional_expression(parser);
    Token *operator;

    while (true)
    {
        operator= match(EQUAL);
        if (operator!= NULL)
        {
            ExprAstNode *right = Parser_assignment_expression(parser);
            expr = EXPR_ASSIGN(operator->pos, .left = expr, .right = right);
            continue;
        }

        operator= match(MUL_ASSIGN, DIV_ASSIGN, MOD_ASSIGN, ADD_ASSIGN, SUB_ASSIGN,
                        LEFT_ASSIGN, RIGHT_ASSIGN, AND_ASSIGN, XOR_ASSIGN, OR_ASSIGN);

        if (operator!= NULL)
        {
            BinaryExprOp op;
            switch (operator->type)
            {
            case MUL_ASSIGN:
                op = BINARY_MUL;
                break;
            case DIV_ASSIGN:
                op = BINARY_DIV;
                break;
            case MOD_ASSIGN:
                op = BINARY_MOD;
                break;
            case ADD_ASSIGN:
                op = BINARY_ADD;
                break;
            case SUB_ASSIGN:
                op = BINARY_SUB;
                break;
            case LEFT_ASSIGN:
                op = BINARY_SLL;
                break;
            case RIGHT_ASSIGN:
                op = BINARY_SLR;
                break;
            case AND_ASSIGN:
                op = BINARY_AND;
                break;
            case XOR_ASSIGN:
                op = BINARY_XOR;
                break;
            case OR_ASSIGN:
                op = BINARY_OR;
                break;
            default:
                break;
            }
            ExprAstNode *right = Parser_assignment_expression(parser);
            expr = desugar_assign(parser, expr, op, right);
        }
        else
        {
            break;
        }
    }
    return expr;
}

static _Bool is_decl(TokenType tok)
{
    switch (tok)
    {
    case VOID:
    case CHAR:
    case SHORT:
    case INT:
    case SIGNED:
    case UNSIGNED:
        return true;
    default:
        return false;
    }
}

static StmtAstNode *expression_statement(Parser *parser)
{
    ExprAstNode *expr = Parser_expression(parser);
    StmtAstNode *stmt = STMT_EXPR(expr->pos, .expr = expr);
    consume(SEMICOLON);
    return stmt;
}

static StmtAstNode *iteration_statement(Parser *parser)
{
    if (match(WHILE))
    {
        consume(LEFT_PAREN);
        ExprAstNode *expr = Parser_expression(parser);
        consume(RIGHT_PAREN);
        StmtAstNode *block = statement(parser);

        return STMT_WHILE(expr->pos, .expr = expr, .block = block);
    }

    return NULL;
}

static StmtAstNode *return_statement(Parser *parser)
{
    StmtAstNode *stmt;
    Token *tok;
    consume(RETURN);

    if ((tok = match(SEMICOLON)))
    {
        stmt = STMT_RETURN(tok->pos);
    }
    else
    {
        ExprAstNode *expr = Parser_expression(parser);
        stmt = STMT_RETURN(expr->pos, .value = expr);
        consume(SEMICOLON);
    }

    return stmt;
}

static StmtAstNode *if_statement(Parser *parser)
{
    /*
     * 'if' '(' expression ')' '{' statement '}'
     * 'if' '(' expression ')' statement 'else' statement
     */
    StmtAstNode *stmt = STMT_IF(consume(IF)->pos);

    consume(LEFT_PAREN);
    stmt->if_statement.expr = Parser_expression(parser);
    consume(RIGHT_PAREN);

    stmt->if_statement.if_arm = statement(parser);

    if (match(ELSE))
    {
        stmt->if_statement.else_arm = statement(parser);
    }

    return stmt;
}

static StmtAstNode *statement(Parser *parser)
{
    if (peek()->type == LEFT_BRACE)
    {
        return Parser_compound_statement(parser);
    }
    if (peek()->type == WHILE)
    {
        return iteration_statement(parser);
    }
    if (peek()->type == RETURN)
    {
        return return_statement(parser);
    }
    if (peek()->type == IF)
    {
        return if_statement(parser);
    }
    return expression_statement(parser);
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
static ParameterListItem *parameter_declaration(Parser *parser)
{
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

static ExprAstNode *initializer(Parser *parser)
{
    return Parser_expression(parser);
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

static Token *match_token(Parser *parser, TokenType *token_types)
{
    for (; *token_types != NAT; token_types++)
    {
        Token *t = peek_token(parser);
        if (t->type != *token_types)
            continue;

        advance_token(parser);
        return t;
    }
    return NULL;
}

/*
 * If the next token does not match, report an error and return false.
 */
static Token *consume_token(Parser *parser, TokenType token_type)
{
    Token *tok = match_token(parser, (TokenType[]){token_type, NAT});
    if (!tok)
    {
        Token *tok = peek_token(parser);

        char err_msg[80];
        snprintf(err_msg, 80, "Expecting '%s', got '%s'", Token_str(token_type),
                 Token_str(tok->type));

        Error_report_error(parser->error_reporter, PARSER, tok->pos, err_msg);
        THROW_ERROR(parser);
    }
    return tok;
}

ExprAstNode *Parser_expression(Parser *parser)
{
    return Parser_assignment_expression(parser);
}

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

static CType *type_name(Parser *parser)
{
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

Parser *Parser_init(Scanner *scanner, ErrorReporter *error_reporter)
{
    Parser *parser = calloc(1, sizeof(Parser));
    parser->scanner = scanner;
    parser->error_reporter = error_reporter;

    parser->next_token[0] = Scanner_get_next(parser->scanner);
    parser->next_token[1] = Scanner_get_next(parser->scanner);
    parser->next_token_index = 0;

    return parser;
}

void Parser_destroy(Parser *parser)
{
    free(parser);
}

DeclAstNode *Parser_translation_unit(Parser *parser)
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

StmtAstNode *Parser_compound_statement(Parser *parser)
{
    StmtAstNode *stmt, *head = NULL, **curr = &stmt;

    Token *brace = consume(LEFT_BRACE);
    while (!match(RIGHT_BRACE))
    {
        if (CATCH_ERROR(parser))
        {
            // Error occurred parsing the following declaration.
            // synchronise on the next semi-colon, or until we reach the RIGHT brace.
            sync(SEMICOLON, RIGHT_BRACE, END_OF_FILE);

            if (peek()->type == SEMICOLON)
                advance();\
            if (peek()->type == END_OF_FILE)
                return NULL;
            continue;
        }

        if (is_decl(peek()->type))
        {
            DeclAstNode *decl = Parser_declaration(parser);
            *curr = STMT_DECL(decl->pos, .decl = decl);
        }
        else
        {
            *curr = statement(parser);
        }
        if (!head)
            head = *curr;
        curr = &((*curr)->next);
    }
    return STMT_BLOCK(brace->pos, .head = head);
}

bool Parser_at_end(Parser * parser)
{
    return peek()->type == END_OF_FILE;
}