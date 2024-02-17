#ifndef AST_H
#define AST_H

#include "../lexer/token.h"
#include "arguments.h"

enum ast_type
{
    AST_COMMAND,
    AST_COMMAND_LIST,
    AST_IF,
    AST_SEMICOLON,
    AST_SINGLE_QUOTE,
    AST_REDIR,
    AST_REDIR_OP,
    AST_PIPE,
    AST_NEGATION,
    AST_WHILE,
    AST_UNTIL,
    AST_AND_IF,
    AST_OR_IF,
    AST_FOR,
    AST_ASSIGNEMENT,
    AST_LONE_ASSIGNMENT, // Only variable assignations; no command
    AST_SUBSHELL,
    AST_FUNCTION,
    AST_CASE,
    AST_CASE_LIST,
    AST_CASE_ITEM

    // to complete
};

union ast_data
{
    struct arguments *argument; // optional argument of the command
    struct ast *ast_arg;
    enum operator_type redir_oper;
    int io_number;
};

struct ast
{
    struct ast *left;
    struct ast *right;
    enum ast_type type;
    union ast_data data; // TODO continue here
};

struct ast *ast_new_ast(enum ast_type type);
struct ast *ast_new_arg(enum ast_type type);

void free_undefined_functions(struct ast *ast);
void ast_free(struct ast *ast);
void ast_free_all(struct ast *ast);

void print_ast(struct ast *node, int space);

#endif /* !AST_H*/
