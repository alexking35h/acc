#include "analysis.h"
#include "ast.h"
#include "token.h"
#include "symbol.h"

static void expr_primary(ExprAstNode*, SymbolTable*);
static void expr_postfix(ExprAstNode*, SymbolTable*);
static void expr_binary(ExprAstNode*, SymbolTable*);
static void expr_unary(ExprAstNode*, SymbolTable*);
static void expr_tertiary(ExprAstNode*, SymbolTable*);
static void expr_cast(ExprAstNode*, SymbolTable*);
static void expr_assign(ExprAstNode*, SymbolTable*);
// static void stmt(StmtAstNode*, SymbolTable*);

static void argument_list_item(ArgumentListItem*, SymbolTable*);

/*
 * Walk the AST.
 */
void analysis_ast_walk(DeclAstNode* root, SymbolTable** global) {
    *global = symbol_table_create(NULL);

    if(root != NULL)
        analysis_ast_walk_decl(root, *global);
}

void analysis_ast_walk_decl(DeclAstNode* node, SymbolTable* tab) {
    // Check if there is already a symbol table entry for this
    // identifier within the current scope.
    // @TODO
    symbol_table_get(tab, node->identifier->lexeme, false);
    Symbol* sym = symbol_table_put(tab, node->identifier->lexeme, node->type);
    node->symbol = sym;

    if(node->next) {
        analysis_ast_walk_decl(node->next, tab);
    }
}

void analysis_ast_walk_expr(ExprAstNode* node, SymbolTable* tab) {
   switch (node->type) {
     case PRIMARY:
         return expr_primary(node, tab);

    case POSTFIX:
        return expr_postfix(node, tab);

    case BINARY:
        return expr_binary(node, tab);

     case UNARY:
         return expr_unary(node, tab);

    case TERTIARY:
        return expr_tertiary(node, tab);
    
    case CAST:
        return expr_cast(node, tab);

    case ASSIGN:
        return expr_assign(node, tab);
   }
}

static void expr_primary(ExprAstNode* node, SymbolTable* tab) {
    Symbol* sym = symbol_table_get(tab, node->primary.identifier->lexeme, true);
    node->primary.symbol = sym;
}

static void expr_postfix(ExprAstNode* node, SymbolTable* tab) {
    analysis_ast_walk_expr(node->postfix.left, tab);
    if(node->postfix.index_expression) {
        analysis_ast_walk_expr(node->postfix.index_expression, tab);
    } else {
        argument_list_item(node->postfix.args, tab);
    }
}

static void expr_binary(ExprAstNode* node, SymbolTable* tab) {
    analysis_ast_walk_expr(node->binary.left, tab);
    analysis_ast_walk_expr(node->binary.right, tab);
}

static void expr_unary(ExprAstNode* node, SymbolTable* tab) {
    analysis_ast_walk_expr(node->unary.right, tab);
}

static void expr_tertiary(ExprAstNode* node, SymbolTable* tab) {
    analysis_ast_walk_expr(node->tertiary.condition_expr, tab);
    analysis_ast_walk_expr(node->tertiary.expr_true, tab);
    analysis_ast_walk_expr(node->tertiary.expr_false, tab);
}

static void expr_cast(ExprAstNode* node, SymbolTable* tab) {
    analysis_ast_walk_expr(node->cast.right, tab);
}

static void expr_assign(ExprAstNode* node, SymbolTable* tab) {
    analysis_ast_walk_expr(node->assign.left, tab);
    analysis_ast_walk_expr(node->assign.right, tab);
}

static void argument_list_item(ArgumentListItem* arg, SymbolTable* tab) {
    if(NULL == arg) return;

    analysis_ast_walk_expr(arg->argument, tab);
    argument_list_item(arg->next, tab);
}
