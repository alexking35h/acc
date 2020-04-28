#include "analysis.h"
#include "ast.h"
#include "token.h"
#include "symbol.h"

#include <string.h>
#include <stdio.h>

static void walk_expr(ExprAstNode*, SymbolTable*);
static void walk_decl(DeclAstNode*, SymbolTable*);
static void walk_stmt(StmtAstNode*, SymbolTable*);

static void walk_expr_primary(ExprAstNode* node, SymbolTable* tab) {
    Symbol* sym = symbol_table_get(tab, node->primary.identifier->lexeme, true);

    if(sym == NULL) {
        char err[50];
        snprintf(err, 50, "Undeclared identifier '%s'", node->primary.identifier->lexeme);
        Error_report_error(ANALYSIS, -1, err);
    }
    node->primary.symbol = sym;
}

static void argument_list_item(ArgumentListItem* arg, SymbolTable* tab) {
    if(NULL == arg) return;

    walk_expr(arg->argument, tab);
    argument_list_item(arg->next, tab);
}

static void walk_expr_postfix(ExprAstNode* node, SymbolTable* tab) {
    walk_expr(node->postfix.left, tab);
    if(node->postfix.index_expression) {
        walk_expr(node->postfix.index_expression, tab);
    } else {
        argument_list_item(node->postfix.args, tab);
    }
}

static void walk_expr_binary(ExprAstNode* node, SymbolTable* tab) {
    walk_expr(node->binary.left, tab);
    walk_expr(node->binary.right, tab);
}

static void walk_expr_unary(ExprAstNode* node, SymbolTable* tab) {
    walk_expr(node->unary.right, tab);
}

static void walk_expr_tertiary(ExprAstNode* node, SymbolTable* tab) {
    walk_expr(node->tertiary.condition_expr, tab);
    walk_expr(node->tertiary.expr_true, tab);
    walk_expr(node->tertiary.expr_false, tab);
}

static void walk_expr_cast(ExprAstNode* node, SymbolTable* tab) {
    walk_expr(node->cast.right, tab);
}

static void walk_expr_assign(ExprAstNode* node, SymbolTable* tab) {
    walk_expr(node->assign.left, tab);
    walk_expr(node->assign.right, tab);
}


static void walk_expr(ExprAstNode* node, SymbolTable* tab) {
   switch (node->type) {
     case PRIMARY:
         return walk_expr_primary(node, tab);

    case POSTFIX:
        return walk_expr_postfix(node, tab);

    case BINARY:
        return walk_expr_binary(node, tab);

     case UNARY:
         return walk_expr_unary(node, tab);

    case TERTIARY:
        return walk_expr_tertiary(node, tab);
    
    case CAST:
        return walk_expr_cast(node, tab);

    case ASSIGN:
        return walk_expr_assign(node, tab);
   }
}

static void walk_decl(DeclAstNode* node, SymbolTable* tab) {
    // Check if there is already a symbol table entry for this
    // identifier within the current scope.
    symbol_table_get(tab, node->identifier->lexeme, false);
    Symbol* sym = symbol_table_put(tab, node->identifier->lexeme, node->type);
    node->symbol = sym;

    if(node->type->type == TYPE_FUNCTION && node->body) {
        walk_stmt(node->body, tab);
    } else if (node->initializer) {
        walk_expr(node->initializer, tab);
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
    walk_expr(node->expr.expr, tab);
}

static void walk_stmt_ret(StmtAstNode* node, SymbolTable* tab) {
    walk_expr(node->return_jump.value, tab);
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
void analysis_ast_walk(DeclAstNode* root, SymbolTable** global) {
    *global = symbol_table_create(NULL);

    if(root != NULL)
        walk_decl(root, *global);
}