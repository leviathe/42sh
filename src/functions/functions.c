#define _POSIX_C_SOURCE 200809L

#include "functions.h"

#include <stdlib.h>
#include <string.h>

static struct fun *functions = NULL;
static struct fun *to_be_freed = NULL;

static struct fun *fun_new(const char *name, struct ast *ast)
{
    struct fun *f = malloc(sizeof(struct fun));
    f->name = name == NULL ? NULL : strdup(name);
    f->fun = ast;
    f->next = NULL;
    f->nesting = 0;
    return f;
}

static void add_to_be_freed(struct ast *fun)
{
    struct fun *f = fun_new(NULL, fun);
    f->next = to_be_freed;
    to_be_freed = f;
}

void free_to_be_freed(void)
{
    for (struct fun *f = to_be_freed; f != NULL; f = f->next)
    {
        free_undefined_functions(f->fun);
    }
    while (to_be_freed != NULL)
    {
        struct fun *save = to_be_freed;
        to_be_freed = to_be_freed->next;
        free(save->name);
        ast_free(save->fun);
        free(save);
    }
}

void add_fun(const char *name, struct ast *fun, char nesting)
{
    struct fun *existing = get_fun(name);
    if (!existing)
    {
        existing = fun_new(name, fun);
        existing->next = functions;
        functions = existing;
    }
    else
    {
        if (!existing->nesting)
            add_to_be_freed(existing->fun);
        existing->fun = fun;
    }
    existing->nesting = nesting;
}

struct fun *get_fun(const char *name)
{
    struct fun *f = functions;
    while (f != NULL)
    {
        if (!strcmp(name, f->name))
            return f;
        f = f->next;
    }
    return NULL;
}

struct ast *get_fun_ast(const char *name)
{
    struct fun *f = get_fun(name);
    if (f != NULL)
        return f->fun;
    return NULL;
}

void del_fun(const char *name)
{
    struct fun *f = functions;
    struct fun *prev = NULL;
    while (f != NULL && strcmp(f->name, name))
    {
        prev = f;
        f = f->next;
    }

    if (f != NULL)
    {
        free(f->name);
        // ast_free(f->fun);
        if (prev != NULL)
            prev->next = f->next;
        else
            functions = f->next;
        // add_to_be_freed(f->fun);
        free(f);
    }
}

void free_functions(void)
{
    for (struct fun *f = functions; f != NULL; f = f->next)
    {
        free_undefined_functions(f->fun);
    }
    while (functions != NULL)
    {
        struct fun *save = functions;
        functions = functions->next;
        free(save->name);
        ast_free(save->fun);
        free(save);
    }
}
