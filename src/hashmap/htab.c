#include "htab.h"

#include <err.h>
#include <string.h>

uint32_t hash(char *key)
{
    uint32_t h = 0;

    while (*key)
    {
        h += *(key++);
        h += h << 10;
        h ^= h >> 6;
    }

    h += h << 3;
    h ^= h >> 11;
    h += h << 15;

    return h;
}

struct htab *htab_new(void)
{
    struct htab *ht = calloc(1, sizeof(struct htab));

    if (ht == NULL)
        errx(1, "Not enough memory!");

    ht->capacity = 4;
    ht->size = 0;
    ht->data = calloc(sizeof(struct pair), ht->capacity);

    if (ht->data == NULL)
        errx(1, "Not enough memory!");

    return ht;
}

struct pair *pair_create(uint32_t hkey, struct word key, struct word value)
{
    // Create a new empty pair.
    struct pair *new_pair = malloc(sizeof(struct pair));
    if (new_pair == NULL)
        errx(1, "Not enough memory!");

    // Initialize the new pair.
    new_pair->hkey = hkey;
    new_pair->key = key;
    new_pair->value = value;
    return new_pair;
}

int pair_update(struct pair *p, struct word value)
{
    free(p->value.data);
    p->value = value;
    return 0;
}

void pair_free(struct pair *p)
{
    free(p->key.data);
    free(p->value.data);
    free(p);
}

void htab_clear(struct htab *ht)
{
    for (size_t i = 0; i < ht->capacity; i++)
    {
        struct pair *p = (ht->data + i)->next;
        struct pair *q;

        while (p != NULL)
        {
            q = p;
            p = p->next;
            pair_free(q);
        }
    }

    ht->size = 0;
}

void htab_free(struct htab *ht)
{
    htab_clear(ht);
    free(ht->data->key.data);
    free(ht->data->value.data);
    free(ht->data);
    free(ht);
}

struct pair *htab_get(struct htab *ht, struct word key)
{
    // Get the hash value of the key.
    uint32_t hkey = hash(key.data);

    // Get the index.
    size_t index = hkey % ht->capacity;

    // Get the sentinel.
    struct pair *sentinel = ht->data + index;

    // If the pair is in the list, return it.
    // A pair is in the list:
    // if the hash values are equal,
    // and if the keys are equal
    // (it is required to test the keys in case of collisions).
    for (struct pair *p = sentinel->next; p != NULL; p = p->next)
        if (hkey == p->hkey)
            if (strcmp(key.data, p->key.data) == 0)
                return p;

    // If the pair is not in the list, return NULL.
    return NULL;
}

int htab_insert(struct htab *ht, struct word key, struct word value)
{
    // Get the hash value of the key.
    uint32_t hkey = hash(key.data);

    // Get the index.
    size_t index = hkey % ht->capacity;

    // Get the sentinel.
    struct pair *sentinel = ht->data + index;

    // If the pair is already in the list, return 0.
    // A pair is already in the list:
    // if the hash values are equal,
    // and if the keys are equal
    // (it is required to test the keys in case of collisions).
    for (struct pair *p = sentinel->next; p != NULL; p = p->next)
        if (hkey == p->hkey && strcmp(key.data, p->key.data) == 0)
            return pair_update(p, value);

    // If it is the first pair in the list, increment the size.
    if (sentinel->next == NULL)
        ht->size++;

    // Create a new empty pair.
    struct pair *new_pair = pair_create(hkey, key, value);
    new_pair->next = sentinel->next;

    // Insert the new pair into the list.
    sentinel->next = new_pair;

    // If the new ratio is greater than 75%.
    if (100 * ht->size / ht->capacity > 75)
    {
        // Copy the current hash table (shallow copy).
        struct htab old = *ht;

        // Multiply by two the capacity.
        ht->capacity *= 2;

        // Reset the size to zero.
        ht->size = 0;

        // Allocate a new memory space.
        ht->data = calloc(sizeof(struct pair), ht->capacity);
        if (ht->data == NULL)
            errx(1, "not enough memory!");

        // Copy each pair of the previous hash table
        // into the current hash table.
        for (size_t i = 0; i < old.capacity; i++)
        {
            for (struct pair *p = old.data[i].next; p != NULL; p = p->next)
            {
                // Get the new index.
                index = p->hkey % ht->capacity;

                // Get the sentinel.
                struct pair *sentinel = ht->data + index;

                // If it is the first pair in the item, increment the size.
                if (sentinel->next == NULL)
                    ht->size++;

                // Create a new empty pair.
                struct pair *new_pair = pair_create(hkey, key, value);
                new_pair->next = sentinel->next;

                // Append the new pair to the list.
                sentinel->next = new_pair;
            }
        }
        htab_clear(&old);
        pair_free(old.data);
    }
    return 1;
}

void htab_remove(struct htab *ht, struct word key)
{
    // Get the hash value, the index and sentinel.
    uint32_t hkey = hash(key.data);
    size_t index = hkey % ht->capacity;
    struct pair *sentinel = ht->data + index;

    // If the pair is in the list, remove it and free it.
    for (struct pair *p = sentinel; p->next != NULL; p = p->next)
    {
        // A pair is in the list if the hash values are equal.
        if (hkey == p->next->hkey)
        {
            // And if the keys are equal.
            // (It is required to test the keys in case of collisions.)
            if (strcmp(key.data, p->next->key.data) == 0)
            {
                // Remove the pair from the list.
                struct pair *to_remove = p->next;
                p->next = to_remove->next;

                // If the cell becomes unused, decrement the size.
                if (sentinel->next == NULL)
                    ht->size--;

                // Free the pair.
                pair_free(to_remove);
                return;
            }
        }
    }
}
