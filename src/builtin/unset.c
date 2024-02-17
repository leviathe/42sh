#define _POSIX_C_SOURCE 200809L

#include "unset.h"

#include <stdio.h>
#include <string.h>

#include "../functions/functions.h"
#include "../variable/variable.h"

// init option of unset to 0
static struct unset_option init_option(void)
{
    struct unset_option op;
    op.f = 1;
    op.v = 1;
    return op;
}

// read the arguments and init the correct variable
static struct unset_option unset_handle_arg(char **arg)
{
    struct unset_option op = init_option();
    int ind = 0;
    while (arg[ind] != NULL && arg[ind][0] == '-')
    {
        for (size_t j = 1; j < strlen(arg[ind]); j++)
        {
            switch (arg[ind][j])
            {
            case 'v':
                op.f = 0;
                break;
            case 'f':
                op.v = 0;
                break;
            default:
                return op;
            }
        }
        ind++;
    }
    return op;
}

void unset(char **arg)
{
    struct unset_option op = unset_handle_arg(arg);
    int ind = 0;
    while (arg[ind] != NULL && arg[ind][0] == '-')
    {
        if (arg[ind][1] != 'f' && arg[ind][1] != 'v')
            break;
        ind++;
    }
    while (arg[ind] != NULL)
    {
        struct variable *var = get_var(strdup(arg[ind]));
        if (var != NULL)
        {
            if (op.v)
            {
                if (var->is_exported)
                    unsetenv(arg[ind]);
                var->is_exported = 0;
                var_assignement_word(arg[ind], "");
                ind++;
                continue;
            }
        }
        if (op.f)
            del_fun(arg[ind]);
        ind++;
    }
}
