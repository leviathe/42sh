#define _POSIX_C_SOURCE 200809L

#include "arguments.h"

#include <string.h>

#define INITIAL_SIZE 4

struct arguments *arguments_new(void)
{
    struct arguments *a = malloc(sizeof(struct arguments));
    a->size = INITIAL_SIZE;
    a->index = 0;
    a->args = calloc(INITIAL_SIZE, sizeof(struct list *));
    return a;
}

void arguments_add(struct arguments *args, struct list *a)
{
    args->args[args->index] = a;
    args->index += 1;
    if (args->index >= args->size)
    {
        args->size *= 2;
        args->args = realloc(args->args, args->size * sizeof(struct arguments));
    }
    args->args[args->index] = NULL;
}

static struct list *list_copy(struct list *l)
{
    if (l == NULL)
        return NULL;
    struct list *res = malloc(sizeof(struct list));
    res->word.data = strdup(l->word.data);
    res->word.context = l->word.context;
    res->word.exp = l->word.exp;
    res->next = list_copy(l->next);
    return res;
}

struct arguments *arguments_copy(struct arguments *args)
{
    struct arguments *copy = malloc(sizeof(struct arguments));
    copy->size = args->size;
    copy->index = args->index;
    copy->args = calloc(copy->size, sizeof(char *)); // Auto null terminated
    for (size_t i = 0; args->args[i] != NULL; i++)
    {
        copy->args[i] = list_copy(args->args[i]);
    }
    return copy;
}

void arguments_clear(struct arguments *args)
{
    for (size_t i = 0; i < args->index; i++)
        list_destroy(args->args[i]);
    args->size = 0;
}

void arguments_free(struct arguments *args)
{
    // for (size_t i = 0; i < args->index; i++)
    //     free(args->args[i]);
    free(args->args);
    free(args);
}
