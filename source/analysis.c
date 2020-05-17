#include "analysis.h"
#include "ast.h"
#include "token.h"
#include "symbol.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/*
 * Walk Expression AST Nodes. Parsing expressions often requires getting type
 * information from child nodes (e.g., a+b requires type information for 'a' and 'b').
 * Therefore, all expressions that walk expression types return a pointer to a type.
 */
static CType *walk_expr(ExprAstNode*, SymbolTable*, _Bool need_lvalue);

static void walk_decl(DeclAstNode*, SymbolTable*);
static void walk_stmt(StmtAstNode*, SymbolTable*);

static CType int_type = {
    TYPE_BASIC,
    .basic.type_specifier = TYPE_SIGNED_INT
};

// Binary nodes include additive (+,-), multiplicative (*,/,%), bitwise operations (<<, >>, &, |),
// comparison operators (<=, <, >, >=, ==, !=) and logical operators (&&, ||). These all have different
// operand constraints and semantics:
// - Can operands be basic/pointer?
// - Must operands be compatible?
// - What is the type for this expression?
//These constraints are described in the binary_op_requirements array. This is used in walk_expr_binary().
typedef struct OpRequirements {
    TokenType op;

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

static OpRequirements binary_op_requirements[] = {

    // '+': both operands arithmetic, or one is pointer.
    {PLUS, true, true, true, NULL},
    {PLUS, true, false, false, NULL},
    {PLUS, false, true, false, NULL},

    // '-' both operands can be arithmetic, pointer, or left is pointer, right is arithmetic.
    {MINUS, true, true, true, NULL},
    {MINUS, false, true, false, NULL},
    {MINUS, false, false, true, NULL},

    // '*' - both operands must be arithmetic
    {STAR, true, true, true, NULL},

    // '/' - both operands must be arithmetic
    {SLASH, true, true, true, NULL},

    // '%' - both operands must be arithmetic
    {PERCENT, true, true, true, NULL},

    // <, <=, >, >= - both operands must be arithmetic, or both pointers to compatible types
    {LESS_THAN, true, true, true, &int_type},
    {LESS_THAN, false, false, true, &int_type},
    {LE_OP, true, true, true, &int_type},
    {LE_OP, false, false, true, &int_type},
    {GREATER_THAN, true, true, true, &int_type},
    {GREATER_THAN, false, false, true, &int_type},
    {GE_OP, true, true, true, &int_type},
    {GE_OP, false, false, true, &int_type},

    // ==, != - both operands must be arithmetic, or both are pointers to compatible types.
    {EQ_OP, true, true, true, &int_type},
    {EQ_OP, false, false, false, &int_type},
    {NE_OP, true, true, true, &int_type},
    {NE_OP, false, false, true, &int_type},

    // &, |, ^ - both operands must be arithmetic.
    {AMPERSAND, true, true, true, NULL},
    {BAR, true, true, true, NULL},
    {CARET, true, true, true, NULL},

    // &&, || - both operands must be scalar (arithmetic and pointer)
    {AND_OP, true, true, false, &int_type},
    {AND_OP, true, false, false, &int_type},
    {AND_OP, false, true, false, &int_type},
    {AND_OP, false, false, false, &int_type},
    {OR_OP, true, true, false, &int_type},
    {OR_OP, true, false, false, &int_type},
    {OR_OP, false, true, false, &int_type},
    {OR_OP, false, false, false, &int_type},

    {NAT, false, false, NULL}
};

static ExprAstNode* create_cast(ExprAstNode* node, CType* type) {
    ExprAstNode* cast_node = calloc(1, sizeof(ExprAstNode));
    cast_node->type = CAST;
    cast_node->cast.type = type;
    cast_node->cast.right = node;

    return cast_node;
}

static _Bool check_assign_cast(ExprAstNode** node, CType* left, CType* right) {
    // Check if type 'right' can be validly assigned to 'left', and create
    // cast if required. Return false if not.
    CType* l = left, *r = right;

    // Check if left and right are pointers to compatible types.
    while(CTYPE_IS_POINTER(l) && CTYPE_IS_POINTER(r)) {
        l = l->derived.type;
        r = r->derived.type;
    }

    // Check if the types are compatible
    if(CTYPE_IS_BASIC(l) && CTYPE_IS_BASIC(r)) {
        // Simple assignment: the left has arithmetic type, and the right has arithmetic type.
        if(l->basic.type_specifier != r->basic.type_specifier)
            *node = create_cast((*node)->assign.right, left);
        return true;
    } else if (CTYPE_IS_FUNCTION(l) && CTYPE_IS_FUNCTION(r)) {
        // Todo: function prototypes are the same.
        return true;
    }
    return false;
}

static CType *integer_promote(ExprAstNode **node, CType *ctype) {
    // Integer promotion (6.3.1.1 (2)) is performed on operands of arithmetic operations,
    // whose type is integer with rank less than signed/unsigned int. If an int can 
    // represent all values of the original type, the value is converted to an int
    // (We assume that this is always the case in ACC).

    if(ctype == NULL || !CTYPE_IS_BASIC(ctype)) {
        return ctype;
    }

    switch (ctype->basic.type_specifier) {
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
    CType* cast_type = calloc(1, sizeof(CType));
    cast_type->type = TYPE_BASIC;
    cast_type->basic.type_specifier = TYPE_SIGNED_INT;

    *node = create_cast(*node, cast_type);
    return cast_type;
}

static CType* type_conversion(ExprAstNode **node_a, CType *ctype_a, ExprAstNode **node_b, CType *ctype_b){
    // Usual Arithmetic Conversions (6.3.1.8) is performed to find a common type 
    // in arithmetic operations. For integers:
    // 1. If both operands have the same sign, the operand with the lower rank is converted.
    // 2. If the unsigned operand has rank >= the signed operand, the signed operand is converted.
    // 3. If the operand with the signed type can represent all values of the unsigned type,
    //    then the unsigned operand is converted to the signed operand type.
    // 
    // ctype_rank implements (2) by by ensuring that the unsigned rank > signed rank, for the same type.
    // ACC assumes that (3) is always true whenever casting to a signed int with a greater rank.
    if(ctype_a->basic.type_specifier == ctype_b->basic.type_specifier)
        return ctype_a;

    ExprAstNode **cast_expr = ctype_rank(ctype_a) < ctype_rank(ctype_b) ? node_a : node_b;
    CType *cast_type = ctype_rank(ctype_a) < ctype_rank(ctype_b) ? ctype_b : ctype_a;

    *cast_expr = create_cast(*cast_expr, cast_type);
    return cast_type;
}

static CType *walk_expr_primary(ExprAstNode* node, SymbolTable* tab, _Bool need_lvalue) {
    if(node->primary.constant) {
        if(need_lvalue) {
            Error_report_error(ANALYSIS, -1, "Invalid lvalue");
        }
        CType *constant_ctype = calloc(1, sizeof(CType));
        constant_ctype->type = TYPE_BASIC;
        constant_ctype->basic.type_specifier = TYPE_SIGNED_INT;
        return constant_ctype;
    } else if (node->primary.string_literal) {
        if(need_lvalue) {
            Error_report_error(ANALYSIS, -1, "Invalid lvalue");
        }
        CType *char_ctype = calloc(1, sizeof(CType));
        char_ctype->type = TYPE_BASIC;
        char_ctype->basic.type_specifier = TYPE_UNSIGNED_CHAR;

        CType *ptr_ctype = calloc(1, sizeof(CType));
        ptr_ctype->type = TYPE_POINTER;
        ptr_ctype->derived.type = char_ctype;
        return ptr_ctype;
    }

    Symbol* sym = symbol_table_get(tab, node->primary.identifier->lexeme, true);
    if(sym == NULL) {
        char err[50];
        snprintf(err, 50, "Undeclared identifier '%s'", node->primary.identifier->lexeme);
        Error_report_error(ANALYSIS, -1, err);
        return NULL;
    }
    node->primary.symbol = sym;
    return node->primary.symbol->type;
}

static void walk_argument_list(ParameterListItem* params, ArgumentListItem* arguments, SymbolTable* tab) {
    int arg_count = 0, param_count = 0;

    for(;arguments && params; arguments = arguments->next, params = params->next) {
        arg_count++;
        param_count++;
        CType* param_type = params->type;
        CType* arg_type = walk_expr(arguments->argument, tab, false);

        if(!check_assign_cast(&arguments->argument, param_type, arg_type)) {
            char err[256];
            int n = snprintf(err, 256, "Incompatible argument type. Cannot pass type '");
            n += ctype_str(err + n, sizeof(err) - n, arg_type);
            n += snprintf(err + n, sizeof(err) - n, "' to type '");
            n += ctype_str(err + n, sizeof(err) - n, param_type);
            n += snprintf(err + n, sizeof(err) - n, "'");
            Error_report_error(ANALYSIS, -1, err);
        }
    }

    if(params || arguments) {
        for(;arguments;arguments = arguments->next) arg_count++;
        for(;params;params = params->next) param_count++;

        char *err = calloc(128, sizeof(char));
        snprintf(err, 128, "Invalid number of arguments to function. Expected %d, got %d",
            param_count, arg_count);
        Error_report_error(ANALYSIS, -1, err);
    }
}

static CType *walk_expr_postfix(ExprAstNode* node, SymbolTable* tab, _Bool need_lvalue) {
    CType *pf = walk_expr(node->postfix.left, tab, false);
    if(!pf) return NULL;

    // The only implemented postfix expression is a function call.
    if(CTYPE_IS_FUNCTION(pf)) {
        walk_argument_list(pf->derived.params, node->postfix.args, tab);
        return pf->derived.type;
    } else {
        Error_report_error(ANALYSIS, -1, "Not a function");
    }
    return NULL;
}

static CType *walk_expr_binary(ExprAstNode* node, SymbolTable* tab, _Bool need_lvalue) {
    if(need_lvalue) Error_report_error(ANALYSIS, -1, "Invalid lvalue");
    CType *left = walk_expr(node->binary.left, tab, false);
    CType *right = walk_expr(node->binary.right, tab, false);

    char err_string[100];

    // The only valid operands to binary operators are scalar (pointer/basic)
    if(!CTYPE_IS_SCALAR(left) || !CTYPE_IS_SCALAR(right)) goto error;
    
    for(OpRequirements *req = binary_op_requirements;req->op != NAT;req++) {
        if(req->op != node->binary.op->type) continue;

        if((req->left_basic && !CTYPE_IS_BASIC(left))
            || (req->right_basic && !CTYPE_IS_BASIC(right))) continue;
        
        if(req->left_basic && req->right_basic) {
            CType *expr_type;
            if(req->compatible) {
                left = integer_promote(&node->binary.left, left);
                right = integer_promote(&node->binary.right, right);
                expr_type = type_conversion(
                    &node->binary.left, 
                    left,
                    &node->binary.right,
                    right
                );
            }
            return req->expr_type ? req->expr_type : expr_type;

        } else if (!req->left_basic && !req->right_basic) {
            if(req->compatible && !ctype_pointers_compatible(left, right)) break;

            // Pointer-Pointer operands (==, !=, &&, ||, <, <=, >, >=) always return an int.
            return req->expr_type;
        } else {
            return req->expr_type ? req->expr_type : (req->left_basic ? right : left);
        }
    }

error:
    snprintf(err_string, 100, "Invalid operand type to binary operator '%s'", node->binary.op->lexeme);
    Error_report_error(ANALYSIS, -1, err_string);
    return NULL;
}

static CType *walk_expr_unary(ExprAstNode* node, SymbolTable* tab, _Bool need_lvalue) {
    CType* ctype = walk_expr(node->unary.right, tab, false);

    if(!ctype)
        return NULL;

    if(node->unary.op->type == STAR) {
        // Pointer dereference.
        if(ctype->type != TYPE_POINTER) {
            Error_report_error(ANALYSIS, -1, "Invalid Pointer dereference");
            return NULL;
        }

        // Return the CType we're dereferencing.
        return ctype->derived.type;
    } else if (node->unary.op->type == AMPERSAND) {
        // Address-of operator.
        CType* addr_of = calloc(1, sizeof(CType));
        addr_of->type = TYPE_POINTER;
        addr_of->derived.type = ctype;

        return addr_of;
    } else {
        // '-', '+', '~', or '!' operators.
        // Test that the operand is of type 'basic' (section 6.5.3.1)
        if(!CTYPE_IS_BASIC(ctype)) {
            char *err = calloc(128, sizeof(char));
            snprintf(err, 128, "Invalid operand to unary operator '%s'", node->unary.op->lexeme);
            Error_report_error(ANALYSIS, -1, err);
        }
        return ctype;
    }
}

static CType *walk_expr_tertiary(ExprAstNode* node, SymbolTable* tab, _Bool need_lvalue) {
    if(need_lvalue) Error_report_error(ANALYSIS, -1, "Invalid lvalue");

    CType* left = walk_expr(node->tertiary.condition_expr, tab, false);
    CType* right = walk_expr(node->tertiary.expr_true, tab, false);

    if(CTYPE_IS_BASIC(left) && CTYPE_IS_BASIC(right)) {
        return left;
    } else if (CTYPE_IS_POINTER(left) && CTYPE_IS_POINTER(right)) {
        return left;
    }

    // Error occurred - the types are not the same.
    Error_report_error(ANALYSIS, -1, "Invalid types in tertiary expression");
    return NULL;
}

static CType *walk_expr_cast(ExprAstNode* node, SymbolTable* tab, _Bool need_lvalue) {
    if(need_lvalue) Error_report_error(ANALYSIS, -1, "Invalid lvalue");
    return walk_expr(node->cast.right, tab, false);
}

static CType *walk_expr_assign(ExprAstNode* node, SymbolTable* tab, _Bool need_lvalue) {
    if(need_lvalue) Error_report_error(ANALYSIS, -1, "Invalid lvalue");
    CType* left = walk_expr(node->assign.left, tab, true);
    CType* right = walk_expr(node->assign.right, tab, false);

    if(!left || !right) 
        return NULL;
    
    if(!check_assign_cast(&node->assign.right, left, right)) {
        char err[256];
        int n = snprintf(err, 256, "Incompatible assignment. Cannot assign type '");
        n += ctype_str(err + n, sizeof(err) - n, right);
        n += snprintf(err + n, sizeof(err) - n, "' to type '");
        n += ctype_str(err + n, sizeof(err) - n, left);
        n += snprintf(err + n, sizeof(err) - n, "'");
        Error_report_error(ANALYSIS, -1, err);
    }
    return left;
}


static CType *walk_expr(ExprAstNode* node, SymbolTable* tab, _Bool need_lvalue) {
   switch (node->type) {
     case PRIMARY:
         return walk_expr_primary(node, tab, need_lvalue);

    case POSTFIX:
        return walk_expr_postfix(node, tab, need_lvalue);

    case BINARY:
        return walk_expr_binary(node, tab, need_lvalue);

     case UNARY:
         return walk_expr_unary(node, tab, need_lvalue);

    case TERTIARY:
        return walk_expr_tertiary(node, tab, need_lvalue);
    
    case CAST:
        return walk_expr_cast(node, tab, need_lvalue);

    case ASSIGN:
        return walk_expr_assign(node, tab, need_lvalue);
   }
   return NULL;
}

static void walk_decl(DeclAstNode* node, SymbolTable* tab) {
    // Check if there is already a symbol table entry for this
    // identifier within the current scope.
    if(symbol_table_get(tab, node->identifier->lexeme, false)) {
        char err[128];
        snprintf(err, sizeof(err), "Previously declared identifier '%s'", node->identifier->lexeme);
        Error_report_error(ANALYSIS, -1, err);

        return;
    }
    Symbol* sym = symbol_table_put(tab, node->identifier->lexeme, node->type);
    node->symbol = sym;

    if(CTYPE_IS_FUNCTION(node->type) && node->body) {
        walk_stmt(node->body, tab);
    } else if (node->initializer) {
        walk_expr(node->initializer, tab, false);
    }

    if(node->next)
        walk_decl(node->next, tab);
}

static void walk_stmt_decl(StmtAstNode* node, SymbolTable* tab) {
    walk_decl(node->decl.decl, tab);
}

static void walk_stmt_block(StmtAstNode* node, SymbolTable* tab) {
    walk_stmt(node->block.head, tab);
}

static void walk_stmt_expr(StmtAstNode* node, SymbolTable* tab) {
    walk_expr(node->expr.expr, tab, false);
}

static void walk_stmt_ret(StmtAstNode* node, SymbolTable* tab) {
    walk_expr(node->return_jump.value, tab, false);
}

static void walk_stmt(StmtAstNode* node, SymbolTable* tab) {
    switch(node->type) {
        case DECL:
            walk_stmt_decl(node, tab);
            break;

        case BLOCK:
            walk_stmt_block(node, tab);
            break;
        
        case EXPR:
            walk_stmt_expr(node, tab);
            break;
        
        case RETURN_JUMP:
            walk_stmt_ret(node, tab);
            break;
    }
    if(node->next)
        walk_stmt(node->next, tab);

}

/*
 * Walk the AST.
 */
void analysis_ast_walk(DeclAstNode* decl, ExprAstNode* expr, StmtAstNode* stmt, SymbolTable* tab) {
    if(decl) {
        walk_decl(decl, tab);
    } else if (expr) {
        walk_expr(expr, tab, false);
    } else if (stmt) {
        walk_stmt(stmt, tab);
    }
}