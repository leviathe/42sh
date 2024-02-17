#include <stdlib.h>

#include "vector.h"

struct vector *vector_init(size_t n)
{
    struct vector *v = malloc(sizeof(struct vector));
    if (!v)
        return v;
    v->size = 0;
    v->capacity = n;
    v->data = calloc(n, sizeof(char));
    if (!v->data)
        return NULL;
    return v;
}

void vector_destroy(struct vector *v)
{
    free(v->data);
    free(v);
    v = NULL;
}
