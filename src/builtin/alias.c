#define _POSIX_C_SOURCE 200809L

#include "alias.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../hashmap/htab.h"

static struct htab *aliases = NULL;

char *get_alias(char *name)
{
    if (aliases == NULL)
        return NULL;

    struct word w = { .data = name,
                      .context = CONTEXT_UNQUOTED,
                      .exp = EXP_NONE };
    w.data = name;
    struct pair *duo = htab_get(aliases, w);
    return duo ? duo->value.data : NULL;
}

static void set_alias(char *name, char *data)
{
    if (aliases == NULL)
        aliases = htab_new();

    struct word wkey = { .data = strdup(name),
                         .context = CONTEXT_UNQUOTED,
                         .exp = EXP_NONE };
    struct word wdata = { .data = strdup(data),
                          .context = CONTEXT_UNQUOTED,
                          .exp = EXP_NONE };

    htab_remove(aliases, wkey);
    htab_insert(aliases, wkey, wdata);
}

/*
** Small function to check if a string is a valid function/variable identifier
** Returns 2 if the delimiter is an equal; 1 if delimiter is \0; 0 if bad name
*/
static int prep_var(char *s, char **data)
{
    int i = 0;
    while (s[i] != '\0'
           && (isalnum(s[i]) || s[i] == '_' || s[i] == '!' || s[i] == '%'
               || s[i] == ',' || s[i] == '@'))
        i++;
    if (s[i] == '=')
    {
        *data = s + i + 1;
        s[i] = '\0'; // Splitting the 2 strings
        return 2; // Code for 'Assignment word'
    }
    return 0;
}

int alias(char **args)
{
    int ret = 0;
    for (size_t i = 0; args[i] != NULL; i++)
    {
        char *data = NULL;
        if (prep_var(args[i], &data))
        {
            set_alias(args[i], data);
        }
        else
        {
            char *val = get_alias(args[i]);
            if (val)
                printf("%s='%s'\n", args[i], val);
            else
            {
                fprintf(stderr, "./42sh: alias: %s: not found\n", args[i]);
                ret = 1;
            }
        }
    }
    return ret;
}

int unalias(char **args)
{
    int ret = 0;
    for (size_t i = 0; args[i] != NULL; i++)
    {
        struct word w = { .data = args[i],
                          .context = CONTEXT_UNQUOTED,
                          .exp = EXP_NONE };
        if (htab_get(aliases, w))
            htab_remove(aliases, w);
        else
        {
            fprintf(stderr, "./42sh: unalias: %s: not found\n", args[i]);
            ret = 1;
        }
    }
    return ret;
}

void free_aliases(void)
{
    if (aliases != NULL)
        htab_free(aliases);
}
