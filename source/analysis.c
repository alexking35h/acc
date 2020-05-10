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

static CType *integer_promote(ExprAstNode **node, CType *ctype) {
    if (ctype == NULL || ctype->type != TYPE_PRIMITIVE) {
        return ctype;
    }

    switch (ctype->primitive.type_specifier) {
        case TYPE_UNSIGNED_INT:
        case TYPE_SIGNED_INT:
            return ctype;

        case TYPE_SIGNED_LONG_INT:
        case TYPE_UNSIGNED_LONG_INT:
        case TYPE_FLOAT:
        case TYPE_VOID:
        case TYPE_DOUBLE:
            return ctype;
        
        case TYPE_SIGNED_CHAR:
        case TYPE_UNSIGNED_CHAR:
        case TYPE_SIGNED_SHORT_INT:
        case TYPE_UNSIGNED_SHORT_INT:
        default:
            break;
    }

    CType *cast_type = calloc(1, sizeof(CType));
    cast_type->type = TYPE_PRIMITIVE;
    cast_type->primitive.type_specifier = TYPE_INT | TYPE_SIGNED;

    ExprAstNode* cast_node = calloc(1, sizeof(ExprAstNode));
    cast_node->type = CAST;
    cast_node->cast.type = cast_type;
    cast_node->cast.right = *node;

    *node = cast_node;
    return cast_type;
}

static CType* type_conversion(ExprAstNode **node_a, CType *ctype_a, ExprAstNode **node_b, CType *ctype_b){
    if(ctype_a->primitive.type_specifier == ctype_b->primitive.type_specifier) {
        return ctype_a;
    }

    ExprAstNode **cast_expr = ctype_rank(ctype_a) < ctype_rank(ctype_b) ? node_a : node_b;
    CType *cast_type = ctype_rank(ctype_a) < ctype_rank(ctype_b) ? ctype_b : ctype_a;

    ExprAstNode *cast_node = calloc(1, sizeof(CType));
    cast_node->type = CAST;
    cast_node->cast.type = cast_type;
    cast_node->cast.right = *cast_expr;

    *cast_expr = cast_node;
    return cast_type;
}

static CType *walk_expr_primary(ExprAstNode* node, SymbolTable* tab, _Bool need_lvalue) {
    if(node->primary.constant) {
        if(need_lvalue) {
            Error_report_error(ANALYSIS, -1, "Invalid lvalue");
        }
        CType *constant_ctype = calloc(1, sizeof(CType));
        constant_ctype->type = TYPE_PRIMITIVE;
        constant_ctype->primitive.type_specifier = TYPE_SIGNED_INT;
        return constant_ctype;
    } else if (node->primary.string_literal) {
        if(need_lvalue) {
            Error_report_error(ANALYSIS, -1, "Invalid lvalue");
        }
        CType *char_ctype = calloc(1, sizeof(CType));
        char_ctype->type = TYPE_PRIMITIVE;
        char_ctype->primitive.storage_class_specifier = TYPE_UNSIGNED_CHAR;

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
    int param_count = 0, arg_count = 0;
    for(;params;params = params->next) param_count++;
    for(;arguments;arguments = arguments->next) {
        arg_count++;
        walk_expr(arguments->argument, tab, false);
    }

    if(param_count != arg_count) {
        char *err = calloc(128, sizeof(char));
        snprintf(err, 128, "Invalid number of arguments to function. Expected %d, got %d",
            param_count, arg_count);
        Error_report_error(ANALYSIS, -1, err);
    }
}

static CType *walk_expr_postfix(ExprAstNode* node, SymbolTable* tab, _Bool need_lvalue) {
    CType *pf = walk_expr(node->postfix.left, tab, false);
    if(pf->type == TYPE_FUNCTION) {
        walk_argument_list(pf->derived.params, node->postfix.args, tab);
        return pf->derived.type;
    }
}

static CType *walk_expr_binary(ExprAstNode* node, SymbolTable* tab, _Bool need_lvalue) {
    if(need_lvalue) Error_report_error(ANALYSIS, -1, "Invalid lvalue");
    CType *left = walk_expr(node->binary.left, tab, false);
    CType *right = walk_expr(node->binary.right, tab, false);

    if(left->type == TYPE_PRIMITIVE && right->type == TYPE_PRIMITIVE) {
        left = integer_promote(&node->binary.left, left);
        right = integer_promote(&node->binary.right, right);
        return type_conversion(
            &node->binary.left, 
            left,
            &node->binary.right,
            right
        );
    } else if (left->type == TYPE_PRIMITIVE && right->type == TYPE_POINTER) {
        return right;
    } else if (left->type == TYPE_POINTER && right->type == TYPE_PRIMITIVE) {
        return left;
    }  
    return NULL;
}

static CType *walk_expr_unary(ExprAstNode* node, SymbolTable* tab, _Bool need_lvalue) {
    CType* ctype = walk_expr(node->unary.right, tab, false);
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
        // Test that the operand is of type 'primitive' (section 6.5.3.1)
        if(ctype->type != TYPE_PRIMITIVE) {
            char *err = calloc(128, sizeof(char));
            snprintf(err, 128, "Invalid operand to unary operator '%s'", node->unary.op->lexeme);
            Error_report_error(ANALYSIS, -1, err);
        }
    }
    return NULL;
}

static CType *walk_expr_tertiary(ExprAstNode* node, SymbolTable* tab, _Bool need_lvalue) {
    if(need_lvalue) Error_report_error(ANALYSIS, -1, "Invalid lvalue");
    walk_expr(node->tertiary.condition_expr, tab, false);
    walk_expr(node->tertiary.expr_true, tab, false);
    walk_expr(node->tertiary.expr_false, tab, false);
    return NULL;
}

static CType *walk_expr_cast(ExprAstNode* node, SymbolTable* tab, _Bool need_lvalue) {
    if(need_lvalue) Error_report_error(ANALYSIS, -1, "Invalid lvalue");
    return walk_expr(node->cast.right, tab, false);
}

static CType *walk_expr_assign(ExprAstNode* node, SymbolTable* tab, _Bool need_lvalue) {
    if(need_lvalue) Error_report_error(ANALYSIS, -1, "Invalid lvalue");
    walk_expr(node->assign.left, tab, true);
    walk_expr(node->assign.right, tab, false);
    return NULL;
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

    if(node->type->type == TYPE_FUNCTION && node->body) {
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