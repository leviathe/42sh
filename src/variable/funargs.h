#ifndef FUNARGS_H
#define FUNARGS_H

#include "../linked_list/list.h"

struct frame
{
    char **args;
    struct frame *next;
    char **quoted_args;
    char *num_args;
};

/*
** Simulates entering a new function frame. Saves the arguments given at the
** top of the frame stack.
** To be called before a function call.
*/
void push_args(char **args);
/*
** Removes the topmost frame from the stack. The one beneath becomes accessible.
** If no more frames available, the function does nothing as it should never
** happen.
** To be called after a function call.
*/
void pop_args(void);
/*
** Clears the entire stack frame. To use in the case of a sudden error, or
** just to be safe at the end of an execution.
*/
void empty_stack(void);
/*
** Gets and returns the number of arguments of the current frame.
** To be used to expand the $# variable.
*/
int get_num_args(void);
/*
** Returns the nth argument of the current frame. If n is too large or no frame
** is present (which should never happen), NULL is returned.
** Note: like other get_ functions, this one does NOT make copies of the
** arguments.
** To use with the expansion of the $n variable.
*/
char *get_arg(unsigned int n);
/*
** Returns all the arguments of the current frame as a list of strings.
** NOTE: the string returned are NOT duplicates.
** Use if YOU implement the handling of multiple strings with quotes and $@.
*/
char **get_args_simple(void);
/*
** Returns all the arguments of the current frames.
** var_char is either '@' or '*', it symbolises which of these variables is
** expanded.
** Note: This function does not make duplicates IN ANY CASE!
** No need to free the arguments; they will be freed when the function ends.
** To use if YOU do NOT implement the handling of multiple strings with quotes.
*/
char **get_args(char var_char, enum context context);

#endif /* !FUNARGS_H */
