#ifndef COMMAND_EXPANSION_H
#define COMMAND_EXPANSION_H

#include "../linked_list/list.h"

/*
** Expands the command represented by the content of data.
** The string that results from that expansion replaces the old content of data,
** which is freed.
** This function needs to be called ONLY when you're sure that the list is
** a command substitution.
** No return value is needed; the result of the execution is always 0
*/
int substitute_command(struct list *data);

#endif /* !COMMAND_EXPANSION_H */
