#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "vector.h"

struct vector *vector_append(struct vector *v, char elt)
{
    if (v)
    {
        if (v->capacity == v->size)
            v = vector_resize(v, v->capacity << 1);
        if (!v)
            return NULL;
        v->data[v->size] = elt;
        v->size++;
        return v;
    }
    return NULL;
}

struct vector *vector_resize(struct vector *v, size_t n)
{
    if (n == 0)
    {
        v->data = realloc(v->data, 0);
        v->size = 0;
        v->capacity = 1;
        return v;
    }
    else if (v->size > n || v->capacity < n)
    {
        v->data = realloc(v->data, sizeof(char) * n);
        if (!v->data)
            return NULL;
        v->capacity = n;
        v->size = v->size < n ? v->size : n;
        return v;
    }
    else
    {
        return v;
    }
}
