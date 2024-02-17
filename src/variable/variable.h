#ifndef VARIABLE_H
#define VARIABLE_H

#include <stdlib.h>

#include "../linked_list/list.h"

struct variable
{
    char *name;
    char *data;
    char is_exported;
};

void list_var_init(void);
void list_var_free(void);

/*
** return the variable corresponding to the name
** name is free during the process
*/
struct variable *get_var(char *name);
/*
** Update the variable pid
*/
void var_update_pid(void);
/*
** Update the variable exit code
*/
void var_update_exit_code(int exit_code);
/*
** assign the varible "name" with its value "data"
*/
int var_assignement_word(char *name, char *data);
/*
** Expend the list **args argument as a list of string
** each string correspond to an argument
*/
char **arguments_expend(struct list **args);

void args_free(char **args);

#endif /* !VARIABLE_H */
