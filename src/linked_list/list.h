#ifndef LIST_H
#define LIST_H

#include <stddef.h>

enum context
{
    CONTEXT_UNQUOTED,
    CONTEXT_SQUOTED,
    CONTEXT_DQUOTED,
};

enum expansion_type
{
    EXP_VARIABLE,
    EXP_COMMAND,
    EXP_ARITHMETIC,
    EXP_NONE,
};

struct word
{
    char *data;
    enum context context;
    enum expansion_type exp;
};

struct list
{
    struct list *next;
    struct word word;
};

/*
** Insert a node containing `value` at the ending of the list.
** Return `NULL` if an error occurred.
*/
struct list *list_append(struct list *list, struct word value);

/*
** Release the memory used by the list.
** Does nothing if `list` is `NULL`.
*/
void list_destroy(struct list *list);

#endif /* !LIST_H */
