#include "ast.h"

#include <stdio.h>

#include "../functions/functions.h"

struct ast *ast_new_ast(enum ast_type type)
{
    struct ast *new = calloc(1, sizeof(struct ast));
    if (!new)
        return NULL;
    new->type = type;
    new->data.ast_arg = NULL;
    new->left = NULL;
    new->right = NULL;
    return new;
}

struct ast *ast_new_arg(enum ast_type type)
{
    struct ast *new = calloc(1, sizeof(struct ast));
    if (!new)
        return NULL;
    new->type = type;
    new->data.argument = arguments_new();
    new->left = NULL;
    new->right = NULL;
    return new;
}

static int free_function_left(struct ast *ast)
{
    struct fun *f = get_fun(ast->data.argument->args[0]->word.data);
    return f == NULL || f->fun != ast->left;
}

static int free_function_left_nested(struct ast *ast)
{
    struct fun *f = get_fun(ast->data.argument->args[0]->word.data);
    return f == NULL || (f->fun != ast->left && f->nesting <= 0);
}

void free_undefined_functions(struct ast *ast)
{
    if (ast == NULL)
        return;

    if (ast->type == AST_FUNCTION)
    {
        if (free_function_left(ast))
            ast_free(ast->left);
        ast->left = NULL;
    }
    else
        free_undefined_functions(ast->left);
    free_undefined_functions(ast->right);
}

/*
** If free_functions is nonzero, functions will be freed.
*/
static void _ast_free(struct ast *ast, int free_functions)
{
    if (ast == NULL)
        return;

    if (ast->type != AST_FUNCTION || free_functions
        || free_function_left_nested(ast))
        _ast_free(ast->left, free_functions);
    ast->left = NULL;

    if (ast->type == AST_IF || ast->type == AST_WHILE || ast->type == AST_UNTIL
        || ast->type == AST_CASE_LIST)
        _ast_free(ast->data.ast_arg, free_functions);
    else if (ast->type != AST_REDIR_OP && ast->type != AST_SUBSHELL
             && ast->type != AST_CASE_LIST)
        arguments_free(ast->data.argument);

    _ast_free(ast->right, free_functions);
    ast->right = NULL;

    free(ast);
}

void ast_free(struct ast *ast)
{
    _ast_free(ast, 0);
}

void ast_free_all(struct ast *ast)
{
    _ast_free(ast, 1);
}
