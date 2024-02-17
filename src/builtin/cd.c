#define _POSIX_C_SOURCE 200809L

#include "cd.h"

#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "../variable/variable.h"

static inline char *concat_pwd(char *arg)
{
    if (arg[0] == '/')
        return strdup(arg);
    char *pwd = strdup(get_var(strdup("PWD"))->data);
    size_t pwdlen = strlen(pwd);
    size_t len = pwdlen + strlen(arg) + 1;
    bool need_slash = pwd[pwdlen - 1] != '/';
    len += need_slash;

    pwd = realloc(pwd, len * sizeof(char));

    if (need_slash)
        pwd[pwdlen] = '/';
    strcpy(&pwd[pwdlen + need_slash], arg);
    return pwd;
}

static inline bool update_pwd(char *path, bool is_printing)
{
    if (chdir(path) == -1)
    {
        perror("Error changing directory");
        free(path);
        return false;
    }
    setenv("OLDPWD", get_var(strdup("PWD"))->data, 1);
    var_assignement_word("OLDPWD", get_var(strdup("PWD"))->data);
    setenv("PWD", path, 1);
    var_assignement_word("PWD", path);
    if (is_printing)
        printf("%s\n", path);
    free(path);
    return true;
}

int cd(char **args)
{
    char *directory = NULL;
    bool is_printing = false;
    if (args[0] && args[1])
    {
        fprintf(stderr, "./42sh: .: bad number of parameters\n");
        return 1;
    }
    if (args[0] == NULL)
        directory = strdup(get_var(strdup("HOME"))->data);
    else if (args[0][0] == '-')
    {
        if (args[0][1])
        {
            fprintf(stderr, "./42sh: cd: invalid option\n");
            return 2;
        }

        directory = strdup(get_var(strdup("OLDPWD"))->data);
        is_printing = true;
    }
    else
        directory = concat_pwd(args[0]);

    if (directory == NULL)
        return 1;

    return !update_pwd(directory, is_printing);
}
