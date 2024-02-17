#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include "../ast/ast.h"

struct fun
{
    char *name;
    struct ast *fun;
    struct fun *next;
    char nesting;
};

void add_fun(const char *name, struct ast *fun, char nesting);
struct fun *get_fun(const char *name);
struct ast *get_fun_ast(const char *name);
void del_fun(const char *name);
void free_functions(void);
void free_to_be_freed(void);

#endif /* !FUNCTIONS_H */
