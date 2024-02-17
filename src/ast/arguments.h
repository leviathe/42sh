#ifndef ARGUMENTS_H
#define ARGUMENTS_H

#include <stdlib.h>

#include "../linked_list/list.h"

struct arguments
{
    struct list **args;
    size_t size;
    size_t index;
};

struct arguments *arguments_new(void);
void arguments_add(struct arguments *args, struct list *a);
struct arguments *arguments_copy(struct arguments *args);
void arguments_clear(struct arguments *args);
void arguments_free(struct arguments *args);

#endif /* !ARGUMENTS_H */
