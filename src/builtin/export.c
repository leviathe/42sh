#define _POSIX_C_SOURCE 200809L

#include "export.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../variable/variable.h"

static struct str_dlist *to_be_exported = NULL;

static void add_to_be_exported(char *s)
{
    struct str_dlist *next = to_be_exported;
    struct str_dlist *l = malloc(sizeof(struct str_dlist));
    l->next = next;
    l->prev = NULL;
    l->data = s;
    to_be_exported = l;
}

int remove_to_be_exported(char *s)
{
    struct str_dlist *l = to_be_exported;
    while (l != NULL && strcmp(s, l->data))
        l = l->next;
    if (l == NULL)
        return 0;
    if (l->prev)
        l->prev->next = l->next;
    if (l->next)
        l->next->prev = l->prev;
    if (l == to_be_exported)
        to_be_exported = l->next;
    free(l->data);
    free(l);
    return 1;
}

void destroy_to_be_exported(void)
{
    struct str_dlist *l = to_be_exported;
    while (l != NULL)
    {
        to_be_exported = l->next;
        free(l->data);
        free(l);
        l = to_be_exported;
    }
}

/*
** Small function to check if a string is a valid function/variable identifier
** Returns 2 if the delimiter is an equal; 1 if delimiter is \0; 0 if bad name
*/
static int prep_var(char *s, char **data)
{
    if (isdigit(s[0]) || !isalpha(s[0]))
        return 0;
    int i = 1;
    while (s[i] != '\0' && (isalnum(s[i]) || s[i] == '_'))
        i++;
    if (s[i] == '=')
    {
        *data = s + i + 1;
        s[i] = '\0'; // Splitting the 2 strings
        return 2; // Code for 'Assignment word'
    }
    return s[i] == '\0';
}

int exportv(char *arg)
{
    char *data = NULL;
    arg = strdup(arg);
    int validity = prep_var(arg, &data);
    if (!validity)
    {
        fprintf(stderr, "./42sh: export: Invalid identifier: %s\n", arg);
        free(arg);
        return 1;
    }
    if (data == NULL) // Is just name
    {
        // arg = strdup(arg);
        // var_assignement_word(arg, "");
        char *dup = strdup(arg);
        struct variable *var = get_var(dup); // Frees the dup arg
        if (var)
        {
            var->is_exported = 1;
            setenv(var->name, var->data, 1); // The 1 is for overriding
            free(arg);
        }
        else
        {
            add_to_be_exported(arg);
        }
    }
    else // Is assignment word
    {
        // char *dup = strdup(arg);
        var_assignement_word(arg, data);
        struct variable *var = get_var(arg); // Frees the dup arg
        if (var)
        {
            var->is_exported = 1;
            setenv(var->name, var->data, 1);
        }
    }
    return 0;
}
