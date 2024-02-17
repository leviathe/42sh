#include "list.h"

#include <stdlib.h>
#include <string.h>

struct list *list_append(struct list *list, struct word value)
{
    struct list *node = malloc(sizeof(struct list));
    if (!node)
        return NULL;

    node->word = value;
    node->next = NULL;
    if (!list)
        return node;

    struct list *tmp = list;
    while (tmp->next)
        tmp = tmp->next;

    tmp->next = node;

    return list;
}

void list_destroy(struct list *list)
{
    if (!list)
        return;

    struct list *node = list;
    struct list *tmp = NULL;
    while (node)
    {
        tmp = node->next;
        free(node->word.data);
        free(node);
        node = tmp;
    }
}
