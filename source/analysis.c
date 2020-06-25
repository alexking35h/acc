#include "analysis.h"
#include "ast.h"
#include "symbol.h"
#include "token.h"
#include "util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * Walk Expression AST Nodes. Parsing expressions often requires getting type
 * information from child nodes (e.g., a+b requires type information for 'a' and 'b').
 * Therefore, all expressions that walk expression types return a pointer to a type.
 */
static CType *walk_expr(ErrorReporter *, ExprAstNode *, SymbolTable *, _Bool);

/* Walk Declaration and Statement nodes. */
static void walk_decl(ErrorReporter *, DeclAstNode *, SymbolTable *, _Bool);
static void walk_stmt(ErrorReporter *, StmtAstNode *, SymbolTable *);

/* Fixed CTypes used throughout analysis */
static CType int_type = {TYPE_BASIC, .basic.type_specifier = TYPE_SIGNED_INT};
static CType char_type = {TYPE_BASIC, .basic.type_specifier = TYPE_UNSIGNED_CHAR};
static CType char_ptr_type = {TYPE_POINTER, .derived.type = &char_type};

// Binary nodes include additive (+,-), multiplicative (*,/,%), bitwise operations (<<,
// >>, &, |), comparison operators (<=, <, >, >=, ==, !=) and logical operators (&&, ||).
// These all have different operand constraints and semantics:
// - Can operands be basic/pointer?
// - Must operands be compatible?
// - What is the type for this expression?
// These constraints are described in the binary_op_requirements array. This is used in
// walk_expr_binary().
typedef struct OpRequirements
{
    BinaryExprOp op;

    // Left/right types must be basic.
    _Bool left_basic;
    _Bool right_basic;

    // Type compatibility requirement, if both operands are pointer/basic:
    // pointer - to same type
    // basic - compatible types, and perform usual conversions
    _Bool compatible;

    // Expression type of this node.
    CType *expr_type;
} OpRequirements;

static const OpRequirements binary_op_requirements[] = {

    // '+': both operands arithmetic, or one is pointer.
    {BINARY_ADD, true, true, true, NULL},
    {BINARY_ADD, true, false, false, NULL},
    {BINARY_ADD, false, true, false, NULL},

    // '-' both operands can be arithmetic, pointer, or left is pointer, right is
    // arithmetic.
    {BINARY_SUB, true, true, true, NULL},
    {BINARY_SUB, false, true, false, NULL},
    {BINARY_SUB, false, false, true, &int_type},

    // '*' - both operands must be arithmetic
    {BINARY_MUL, true, true, true, NULL},

    // '/' - both operands must be arithmetic
    {BINARY_DIV, true, true, true, NULL},

    // '%' - both operands must be arithmetic
    {BINARY_MOD, true, true, true, NULL},

    // '<<', '>>' - both operands must be arithmetic.
    {BINARY_SLL, true, true, true, NULL},
    {BINARY_SLR, true, true, true, NULL},

    // <, <=, >, >= - both operands must be arithmetic, or both pointers to compatible
    // types
    {BINARY_LT, true, true, true, &int_type},
    {BINARY_LT, false, false, true, &int_type},
    {BINARY_LE, true, true, true, &int_type},
    {BINARY_LE, false, false, true, &int_type},
    {BINARY_GT, true, true, true, &int_type},
    {BINARY_GT, false, false, true, &int_type},
    {BINARY_GE, true, true, true, &int_type},
    {BINARY_GE, false, false, true, &int_type},

    // ==, != - both operands must be arithmetic, or both are pointers to compatible
    // types.
    {BINARY_EQ, true, true, true, &int_type},
    {BINARY_EQ, false, false, false, &int_type},
    {BINARY_NE, true, true, true, &int_type},
    {BINARY_NE, false, false, true, &int_type},

    // &, |, ^ - both operands must be arithmetic.
    {BINARY_AND, true, true, true, NULL},
    {BINARY_OR, true, true, true, NULL},
    {BINARY_XOR, true, true, true, NULL},

    // &&, || - both operands must be scalar (arithmetic and pointer)
    {BINARY_AND_OP, true, true, false, &int_type},
    {BINARY_AND_OP, true, false, false, &int_type},
    {BINARY_AND_OP, false, true, false, &int_type},
    {BINARY_AND_OP, false, false, false, &int_type},
    {BINARY_OR_OP, true, true, false, &int_type},
    {BINARY_OR_OP, true, false, false, &int_type},
    {BINARY_OR_OP, false, true, false, &int_type},
    {BINARY_OR_OP, false, false, false, &int_type},
};

static ExprAstNode *create_cast(ExprAstNode *node, CType *type)
{
    ExprAstNode *cast_node = calloc(1, sizeof(ExprAstNode));
    cast_node->type = CAST;
    cast_node->cast.type = type;
    cast_node->cast.right = node;

    return cast_node;
}

static _Bool check_assign_cast(ExprAstNode **node, CType *left, CType *right)
{
    // Check if type 'right' can be validly assigned to 'left', and create
    // cast if required. Return false if not.
    CType *l = left, *r = right;

    // Check if left and right are pointers to compatible types.
    while (CTYPE_IS_POINTER(l) && CTYPE_IS_POINTER(r))
    {
        l = l->derived.type;
        r = r->derived.type;
    }

    // Check if the types are compatible
    if (CTYPE_IS_BASIC(l) && CTYPE_IS_BASIC(r))
    {
        // Simple assignment: the left has arithmetic type, and the right has arithmetic
        // type.
        if (l->basic.type_specifier != r->basic.type_specifier)
            *node = create_cast((*node)->assign.right, left);
        return true;
    }
    else if (CTYPE_IS_FUNCTION(l) && CTYPE_IS_FUNCTION(r))
    {
        // Todo: function prototypes are the same.
        return true;
    }
    return false;
}

static CType *integer_promote(ExprAstNode **node, CType *ctype)
{
    // Integer promotion (6.3.1.1 (2)) is performed on operands of arithmetic operations,
    // whose type is integer with rank less than signed/unsigned int. If an int can
    // represent all values of the original type, the value is converted to an int
    // (We assume that this is always the case in ACC).

    if (ctype == NULL || !CTYPE_IS_BASIC(ctype))
    {
        return ctype;
    }

    switch (ctype->basic.type_specifier)
    {
    case TYPE_UNSIGNED_INT:
    case TYPE_SIGNED_INT:
        return ctype;

    case TYPE_SIGNED_LONG_INT:
    case TYPE_UNSIGNED_LONG_INT:
    case TYPE_VOID:
        return ctype;

    case TYPE_SIGNED_CHAR:
    case TYPE_UNSIGNED_CHAR:
    case TYPE_SIGNED_SHORT_INT:
    case TYPE_UNSIGNED_SHORT_INT:
    default:
        break;
    }
    CType *cast_type = calloc(1, sizeof(CType));
    cast_type->type = TYPE_BASIC;
    cast_type->basic.type_specifier = TYPE_SIGNED_INT;

    *node = create_cast(*node, cast_type);
    return cast_type;
}

static CType *type_conversion(ExprAstNode **node_a, CType *ctype_a, ExprAstNode **node_b,
                              CType *ctype_b)
{
    // Usual Arithmetic Conversions (6.3.1.8) is performed to find a common type
    // in arithmetic operations. For integers:
    // 1. If both operands have the same sign, the operand with the lower rank is
    // converted.
    // 2. If the unsigned operand has rank >= the signed operand, the signed operand is
    // converted.
    // 3. If the operand with the signed type can represent all values of the unsigned
    // type,
    //    then the unsigned operand is converted to the signed operand type.
    //
    // ctype_rank implements (2) by by ensuring that the unsigned rank > signed rank, for
    // the same type. ACC assumes that (3) is always true whenever casting to a signed int
    // with a greater rank.
    if (ctype_a->basic.type_specifier == ctype_b->basic.type_specifier)
        return ctype_a;

    ExprAstNode **cast_expr = ctype_rank(ctype_a) < ctype_rank(ctype_b) ? node_a : node_b;
    CType *cast_type = ctype_rank(ctype_a) < ctype_rank(ctype_b) ? ctype_b : ctype_a;

    *cast_expr = create_cast(*cast_expr, cast_type);
    return cast_type;
}

static CType *walk_expr_primary(ErrorReporter *error, ExprAstNode *node, SymbolTable *tab,
                                _Bool need_lvalue)
{
    if (node->primary.constant || node->primary.string_literal)
    {
        if (need_lvalue)
        {
            Error_report_error(error, ANALYSIS, node->pos, "Invalid lvalue");
        }
        return node->primary.constant ? &int_type : &char_ptr_type;
    }

    Symbol *sym = symbol_table_get(tab, node->primary.identifier->lexeme, true);
    if (sym == NULL)
    {
        char *err =
            STR_CONCAT("Undeclared identifier '", node->primary.identifier->lexeme, "'");
        Error_report_error(error, ANALYSIS, node->pos, err);
        return NULL;
    }
    node->primary.symbol = sym;

    if (node->primary.symbol->type->type == TYPE_ARRAY)
    {
        CType *ptr_type = calloc(1, sizeof(CType));
        ptr_type->type = TYPE_POINTER;
        ptr_type->derived.type = node->primary.symbol->type->derived.type;
        return ptr_type;
    }
    return node->primary.symbol->type;
}

static void walk_argument_list(ErrorReporter *error, ParameterListItem *params,
                               ExprAstNode *call_node, SymbolTable *tab)
{
    int arg_count = 0, param_count = 0;

    ArgumentListItem *arguments = call_node->postfix.args;

    for (; arguments && params; arguments = arguments->next, params = params->next)
    {
        arg_count++;
        param_count++;
        CType *param_type = params->type;
        CType *arg_type = walk_expr(error, arguments->argument, tab, false);

        if (!check_assign_cast(&arguments->argument, param_type, arg_type))
        {
            char *err = STR_CONCAT("Incompatible argument type. Cannot pass type '",
                                   ctype_str(arg_type), "' to type '",
                                   ctype_str(param_type), "'");
            Error_report_error(error, ANALYSIS, arguments->argument->pos, err);
        }
    }

    if (params || arguments)
    {
        for (; arguments; arguments = arguments->next)
            arg_count++;
        for (; params; params = params->next)
            param_count++;

        char err[100];
        snprintf(err, 100, "Invalid number of arguments to function. Expected %d, got %d",
                 param_count, arg_count);
        Error_report_error(error, ANALYSIS, call_node->pos, err);
    }
}

static CType *walk_expr_postfix(ErrorReporter *error, ExprAstNode *node, SymbolTable *tab,
                                _Bool need_lvalue)
{
    CType *pf = walk_expr(error, node->postfix.left, tab, false);
    if (!pf)
        return NULL;

    // The only implemented postfix expression is a function call.
    if (!CTYPE_IS_FUNCTION(pf))
    {
        Error_report_error(error, ANALYSIS, node->pos, "Not a function");
        return NULL;
    }
    walk_argument_list(error, pf->derived.params, node, tab);
    return pf->derived.type;
}

static CType *walk_expr_binary(ErrorReporter *error, ExprAstNode *node, SymbolTable *tab,
                               _Bool need_lvalue)
{
    if (need_lvalue)
    {
        Error_report_error(error, ANALYSIS, node->pos, "Invalid lvalue");
    }
    CType *left = walk_expr(error, node->binary.left, tab, false);
    CType *right = walk_expr(error, node->binary.right, tab, false);

    // The only valid operands to binary operators are scalar (pointer/basic)
    if (!CTYPE_IS_SCALAR(left) || !CTYPE_IS_SCALAR(right))
        goto err;

    for (int i = 0;
         i < sizeof(binary_op_requirements) / sizeof(binary_op_requirements[0]); i++)
    {
        const OpRequirements *req = &binary_op_requirements[i];
        if (req->op != node->binary.op)
            continue;

        if ((req->left_basic && !CTYPE_IS_BASIC(left)) ||
            (req->right_basic && !CTYPE_IS_BASIC(right)))
            continue;

        if (req->left_basic && req->right_basic)
        {
            CType *expr_type;
            if (req->compatible)
            {
                left = integer_promote(&node->binary.left, left);
                right = integer_promote(&node->binary.right, right);
                expr_type =
                    type_conversion(&node->binary.left, left, &node->binary.right, right);
            }
            return req->expr_type ? req->expr_type : expr_type;
        }
        else if (!req->left_basic && !req->right_basic)
        {
            if (req->compatible && !ctype_pointers_compatible(left, right))
                break;

            // Pointer-Pointer operands (==, !=, &&, ||, <, <=, >, >=) always return an
            // int.
            return req->expr_type;
        }
        else
        {
            return req->expr_type ? req->expr_type : (req->left_basic ? right : left);
        }
    }
err : {
    Error_report_error(error, ANALYSIS, node->pos,
                       "Invalid operand type to binary operator");
}
    return NULL;
}

static CType *walk_expr_unary(ErrorReporter *error, ExprAstNode *node, SymbolTable *tab,
                              _Bool need_lvalue)
{
    CType *ctype = walk_expr(error, node->unary.right, tab, false);

    if (!ctype)
        return NULL;

    if (node->unary.op == UNARY_DEREFERENCE)
    {
        // Pointer dereference.
        if (ctype->type != TYPE_POINTER)
        {
            Error_report_error(error, ANALYSIS, node->pos, "Invalid Pointer dereference");
            return NULL;
        }

        // Return the CType we're dereferencing.
        return ctype->derived.type;
    }
    else if (node->unary.op == UNARY_ADDRESS_OF)
    {
        // Address-of operator.
        CType *addr_of = calloc(1, sizeof(CType));
        addr_of->type = TYPE_POINTER;
        addr_of->derived.type = ctype;

        return addr_of;
    }
    else
    {
        // '-', '+', '~', or '!' operators.
        // Test that the operand is of type 'basic' (section 6.5.3.1)
        if (!CTYPE_IS_BASIC(ctype))
        {
            Error_report_error(error, ANALYSIS, node->pos,
                               "Invalid operand to unary operator");
        }
        return ctype;
    }
}

static CType *walk_expr_tertiary(ErrorReporter *error, ExprAstNode *node,
                                 SymbolTable *tab, _Bool need_lvalue)
{
    if (need_lvalue)
    {
        Error_report_error(error, ANALYSIS, node->pos, "Invalid lvalue");
    }

    CType *left = walk_expr(error, node->tertiary.condition_expr, tab, false);
    CType *right = walk_expr(error, node->tertiary.expr_true, tab, false);

    if (CTYPE_IS_BASIC(left) && CTYPE_IS_BASIC(right))
    {
        return left;
    }
    else if (CTYPE_IS_POINTER(left) && CTYPE_IS_POINTER(right))
    {
        return left;
    }

    // Error occurred - the types are not the same.
    Error_report_error(error, ANALYSIS, node->pos,
                       "Invalid types in tertiary expression");
    return NULL;
}

static CType *walk_expr_cast(ErrorReporter *error, ExprAstNode *node, SymbolTable *tab,
                             _Bool need_lvalue)
{
    if (need_lvalue)
    {
        Error_report_error(error, ANALYSIS, node->pos, "Invalid lvalue");
    }
    return walk_expr(error, node->cast.right, tab, false);
}

static CType *walk_expr_assign(ErrorReporter *error, ExprAstNode *node, SymbolTable *tab,
                               _Bool need_lvalue)
{
    if (need_lvalue)
    {
        Error_report_error(error, ANALYSIS, node->pos, "Invalid lvalue");
    }
    CType *left = walk_expr(error, node->assign.left, tab, true);
    CType *right = walk_expr(error, node->assign.right, tab, false);

    if (!left || !right)
        return NULL;

    if (!check_assign_cast(&node->assign.right, left, right))
    {
        char *err = STR_CONCAT("Incompatible assignment. Cannot assign type '",
                               ctype_str(right), "' to type '", ctype_str(left), "'");
        Error_report_error(error, ANALYSIS, node->pos, err);
    }
    return left;
}

static CType *walk_expr(ErrorReporter *error, ExprAstNode *node, SymbolTable *tab,
                        _Bool need_lvalue)
{
    switch (node->type)
    {
    case PRIMARY:
        return walk_expr_primary(error, node, tab, need_lvalue);

    case POSTFIX:
        return walk_expr_postfix(error, node, tab, need_lvalue);

    case BINARY:
        return walk_expr_binary(error, node, tab, need_lvalue);

    case UNARY:
        return walk_expr_unary(error, node, tab, need_lvalue);

    case TERTIARY:
        return walk_expr_tertiary(error, node, tab, need_lvalue);

    case CAST:
        return walk_expr_cast(error, node, tab, need_lvalue);

    case ASSIGN:
        return walk_expr_assign(error, node, tab, need_lvalue);
    }
    return NULL;
}

static void walk_decl_function(ErrorReporter *error, DeclAstNode *node, SymbolTable *tab)
{
    if (node->body)
    {
        walk_stmt(error, node->body, symbol_table_create(tab));
    }
}

static void walk_decl_object(ErrorReporter *error, DeclAstNode *node, SymbolTable *tab)
{
    if (node->initializer)
    {
        CType *type = walk_expr(error, node->initializer, tab, false);

        if (!check_assign_cast(&node->initializer, node->type, type))
        {
            char *err =
                STR_CONCAT("Invalid initializer value. Cannot assign type '",
                           ctype_str(type), "' to type '", ctype_str(node->type), "'");
            Error_report_error(error, ANALYSIS, node->initializer->pos, err);
        }
    }
}

/*
 * Walk declaration. Parameters:
 * error - Instance of ErrorReporter
 * node - DeclAstNode to inspect
 * tab - symbol table
 * tu - translation unit - false if this declaration is within a function body.
 */
static void walk_decl(ErrorReporter *error, DeclAstNode *node, SymbolTable *tab, _Bool tu)
{
    if (CTYPE_IS_FUNCTION(node->type) && !tu)
    {
        // Check if we're trying to define a function within a function.
        char *err = STR_CONCAT("Cannot have nested functions ('",
                               node->identifier->lexeme, "'). Try Rust?");
        Error_report_error(error, ANALYSIS, node->pos, err);
    }
    else if (symbol_table_get(tab, node->identifier->lexeme, false))
    {
        // Check if there is already a symbol table entry for this
        // identifier within the current scope.
        char *err =
            STR_CONCAT("Previously declared identifier '", node->identifier->lexeme, "'");
        Error_report_error(error, ANALYSIS, node->pos, err);
    }
    else
    {
        node->symbol = symbol_table_put(tab, node->identifier->lexeme, node->type);
        node->symbol->type = node->type;

        if (CTYPE_IS_FUNCTION(node->type))
        {
            walk_decl_function(error, node, tab);
        }
        else
        {
            walk_decl_object(error, node, tab);
        }
    }

    if (node->next)
        walk_decl(error, node->next, tab, tu);
}

static void walk_stmt_decl(ErrorReporter *error, StmtAstNode *node, SymbolTable *tab)
{
    walk_decl(error, node->decl.decl, tab, false);
}

static void walk_stmt_block(ErrorReporter *error, StmtAstNode *node, SymbolTable *tab)
{
    walk_stmt(error, node->block.head, tab);
}

static void walk_stmt_expr(ErrorReporter *error, StmtAstNode *node, SymbolTable *tab)
{
    walk_expr(error, node->expr.expr, tab, false);
}

static void walk_stmt_ret(ErrorReporter *error, StmtAstNode *node, SymbolTable *tab)
{
    walk_expr(error, node->return_jump.value, tab, false);
}

static void walk_stmt(ErrorReporter *error, StmtAstNode *node, SymbolTable *tab)
{
    switch (node->type)
    {
    case DECL:
        walk_stmt_decl(error, node, tab);
        break;

    case BLOCK:
        walk_stmt_block(error, node, tab);
        break;

    case EXPR:
        walk_stmt_expr(error, node, tab);
        break;

    case RETURN_JUMP:
        walk_stmt_ret(error, node, tab);
        break;
    }
    if (node->next)
        walk_stmt(error, node->next, tab);
}

/*
 * Walk the AST.
 */
void analysis_ast_walk(ErrorReporter *error, DeclAstNode *decl, ExprAstNode *expr,
                       StmtAstNode *stmt, SymbolTable *tab)
{
    if (decl)
    {
        walk_decl(error, decl, tab, true);
    }
    else if (expr)
    {
        walk_expr(error, expr, tab, false);
    }
    else if (stmt)
    {
        walk_stmt(error, stmt, tab);
    }
}