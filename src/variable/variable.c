#define _POSIX_C_SOURCE 200809L

#include "variable.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "../builtin/export.h"
#include "../linked_list/list.h"
#include "../variable/funargs.h"
#include "command_expansion.h"

static size_t list_capacity;
static size_t list_var_ind;
static struct variable **list_var;

static char *itoa(int nb)
{
    int nb2 = nb;
    int size = (nb2 == 0) + 1;
    while (nb2 != 0)
    {
        nb2 /= 10;
        size++;
    }
    char *data = malloc(size);
    data[size - 1] = '\0';
    int i = size - 2;
    if (nb == 0)
        data[i] = '0';
    while (nb != 0)
    {
        data[i] = nb % 10 + '0';
        nb /= 10;
        i--;
    }
    return data;
}

void list_var_free(void)
{
    for (size_t i = 0; i < list_var_ind; i++)
    {
        free(list_var[i]->data);
        free(list_var[i]->name);
        free(list_var[i]);
    }
    free(list_var);
    destroy_to_be_exported(); // Has to do with export, don't worry about it
}

struct variable *get_var(char *name)
{
    for (size_t i = 0; i < list_var_ind; i++)
    {
        if (strcmp(name, list_var[i]->name) == 0)
        {
            if (strcmp(name, "RANDOM") == 0)
            {
                free(list_var[i]->data);
                list_var[i]->data = itoa(rand() % 32767);
            }
            free(name);
            return list_var[i];
        }
    }
    free(name);
    return NULL; // if NULL, var do not exist
}

static void add_in_list(struct variable *var)
{
    if (list_var_ind >= list_capacity)
    {
        list_capacity *= 2;
        list_var = realloc(list_var, list_capacity * sizeof(struct variable));
        if (list_var == NULL)
            return;
    }
    list_var[list_var_ind] = var;
    list_var_ind++;
    list_var[list_var_ind] = NULL;
}

static void set_var(char *name, char *data)
{
    struct variable *var = get_var(strdup(name));
    if (var == NULL)
    {
        var = malloc(sizeof(struct variable));
        var->name = strdup(name);
        var->data = data;
        var->is_exported = remove_to_be_exported(name);
        if (var->is_exported)
            setenv(name, data, 1); // The 1 is for overwriting
        add_in_list(var);
        return;
    }
    free(var->data);
    var->data = data;
    if (var->is_exported)
        setenv(name, data, 1); // The 1 is for overwriting
}

static int is_ifs(char c, char *needle)
{
    for (size_t j = 0; needle[j]; j++)
    {
        if (c == needle[j])
            return 1;
    }
    return 0;
}

// remove stray spaces
static char *handle_var_space(char *data)
{
    char *ifs = get_var(strdup("IFS"))->data;

    int expend_size = strlen(data);
    char *expend = malloc(sizeof(char) * expend_size + 1);
    int space = 2;
    int j = 0;
    for (size_t i = 0; data[i] != '\0'; i++)
    {
        if (is_ifs(data[i], ifs))
        {
            if (space == 0)
                space = 1;
        }
        else
        {
            if (space == 1)
            {
                expend[j] = ifs[0];
                j++;
            }
            space = 0;
            expend[j] = data[i];
            j++;
        }
    }
    expend[j] = '\0';
    return expend;
}

static char *strcat_expend(char *ret, char *data)
{
    if (ret == NULL)
    {
        ret = data;
        return ret;
    }

    int size = strlen(ret) + strlen(data) + 1;
    ret = realloc(ret, size);
    strcat(ret, data);
    free(data);
    return ret;
}

static char *strchar(char *list, char *needle)
{
    for (size_t i = 0; list[i]; i++)
    {
        for (size_t j = 0; needle[j]; j++)
        {
            if (list[i] == needle[j])
                return &list[i];
        }
    }
    return NULL;
}

static char **handle_list_var(char **ret, char *list, int *ind, int *max_size)
{
    int ind_back = *ind;
    while (list)
    {
        ret[*ind] = list;
        char *ifs = get_var(strdup("IFS"))->data;
        if (ifs == NULL)
        {
            (*ind) += 1;
            if (*max_size - 1 == *ind)
            {
                (*max_size) *= 2;
                ret = realloc(ret, *max_size * sizeof(char *));
            }
            ret[*ind] = NULL;
            break;
        }
        char *back = list;
        list = strchar(list, ifs);
        if (list == back)
        {
            list++;
            continue;
        }
        if (list)
        {
            *list = '\0';
            list++;
            (*ind) += 1;
            if (*max_size - 1 == *ind)
            {
                (*max_size) *= 2;
                ret = realloc(ret, *max_size * sizeof(char *));
            }
            ret[*ind] = NULL;
        }
    }
    list = ret[ind_back];
    for (int i = ind_back; i <= *ind; i++)
        ret[i] = strdup(ret[i]);
    free(list);
    return ret;
}

static char **list_expend(struct list *data)
{
    int ind = 0;
    int max_size = 20;
    char **ret = malloc(max_size * sizeof(char *));
    ret[0] = NULL;
    while (data)
    {
        if (data->word.context == CONTEXT_SQUOTED)
        {
            ret[ind] = strcat_expend(ret[ind], strdup(data->word.data));
        }
        else
        {
            if (data->word.exp == EXP_COMMAND)
                var_update_exit_code(substitute_command(data));
            if (data->word.context == CONTEXT_DQUOTED)
            {
                if (data->word.exp == EXP_VARIABLE)
                {
                    struct variable *var = get_var(strdup(data->word.data));
                    if (var == NULL)
                    {
                        data = data->next;
                        continue;
                    }
                    char *list = strcat_expend(ret[ind], strdup(var->data));
                    if (!strcmp(var->name, "@"))
                        ret = handle_list_var(ret, list, &ind, &max_size);
                    else
                        ret[ind] = list;
                }
                else
                {
                    ret[ind] = strcat_expend(ret[ind], strdup(data->word.data));
                }
            }
            else
            {
                if (data->word.exp == EXP_VARIABLE)
                {
                    struct variable *var = get_var(strdup(data->word.data));
                    if (var == NULL)
                    {
                        data = data->next;
                        continue;
                    }
                    char *list =
                        strcat_expend(ret[ind], handle_var_space(var->data));
                    ret = handle_list_var(ret, list, &ind, &max_size);
                }
                else
                {
                    char *list = strcat_expend(
                        ret[ind], handle_var_space(data->word.data));
                    ret = handle_list_var(ret, list, &ind, &max_size);
                }
            }
        }

        data = data->next;
    }
    ret[ind + 1] = NULL;
    return ret;
}

char **arguments_expend(struct list **args)
{
    int max_size = 20;
    char **ret = malloc(sizeof(char *) * max_size);

    int ind = 0;
    int ind_ret = 0;
    while (args[ind] != NULL)
    {
        char **list = list_expend(args[ind]);
        int i = 0;
        for (; list[i]; i++)
        {
            if (max_size == ind_ret)
            {
                max_size *= 2;
                ret = realloc(ret, sizeof(char *) * max_size);
            }
            ret[ind_ret] = list[i];
            ind_ret++;
        }
        ind++;
        free(list);
    }
    ret[ind_ret] = NULL;

    return ret;
}

int var_assignement_word(char *name, char *data)
{
    if (data == NULL)
        return 0;
    if (data[0] == '\0')
    {
        set_var(name, strdup(""));
        return 0;
    }
    set_var(name, strdup(data));
    return 0; // TODO handle error
}

void args_free(char **args)
{
    int ind = 0;
    while (args[ind] != NULL)
    {
        free(args[ind]);
        ind++;
    }
    free(args);
}

static void *get_var_handle_null_env(const char *env_var)
{
    char *env = getenv(env_var);
    if (env == NULL)
        return strdup("\0");
    return strdup(env);
}

static void set_cwd(char *name)
{
    char *pwd = getcwd(NULL, 0);
    set_var(name, pwd);
    setenv(name, pwd, 1);
}

static void special_var_init(void)
{
    int ind = 0;
    set_var("#", itoa(ind));
    set_var("?", itoa(0));
    set_var("$", itoa(getpid())); // readonly
    set_var("UID", itoa(getuid())); // readonly
    set_var("IFS", strdup("\t\n "));

    srand(time(NULL));
    set_var("RANDOM", itoa(rand() % 32767));
    set_var("HOME", get_var_handle_null_env("HOME"));
    set_var("OLDPWD", get_var_handle_null_env("OLDPWD"));
    set_cwd("PWD");
}

void list_var_init(void)
{
    list_capacity = 50;
    list_var_ind = 0;
    list_var = malloc(list_capacity * sizeof(struct variable *));
    list_var[0] = NULL;
    special_var_init();
}

void var_update_pid(void)
{
    set_var("$", itoa(getpid())); // readonly
}

void var_update_exit_code(int exit_code)
{
    set_var("?", itoa(exit_code));
}
