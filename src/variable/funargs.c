#define _POSIX_C_SOURCE 200809L

#include "funargs.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../variable/variable.h"

static struct frame *frame_stack;

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

void set_list_arg(char **args)
{
    int size = 1;
    for (int i = 0; args[i]; i++)
        size += strlen(args[i]) + 1;
    char *res = malloc(size);
    res[0] = '\0';
    for (int i = 0; args[i]; i++)
    {
        strcat(res, args[i]);
        if (args[i + 1])
            strcat(res, " ");
    }
    var_assignement_word("*", res);
    var_assignement_word("@", res);
    free(res);
}

void set_number_arg(char **args, char **num_args)
{
    int i = 0;
    for (; args[i]; i++)
    {
        char *name = itoa(i + 1);
        var_assignement_word(name, args[i]);
        free(name);
    }
    if (*num_args == NULL)
    {
        *num_args = itoa(i);
    }
    var_assignement_word("#", *num_args);
}

void set_function_args(char **args, char **num_args)
{
    set_list_arg(args);
    if (args)
        set_number_arg(args, num_args);
}

void push_args(char **args)
{
    struct frame *frame = malloc(sizeof(struct frame));
    frame->args = args + 1;
    frame->next = frame_stack;
    frame->num_args = NULL;
    frame_stack = frame;
    frame->quoted_args = malloc(sizeof(char *) * 2);
    frame->quoted_args[0] = NULL;
    frame->quoted_args[1] = NULL;
    set_function_args(args + 1, &frame->num_args);
}

void pop_args(void)
{
    if (frame_stack != NULL)
    {
        struct frame *next = frame_stack->next;
        if (frame_stack->quoted_args[0] != NULL)
            free(frame_stack->quoted_args[0]);
        free(frame_stack->num_args);
        free(frame_stack->quoted_args);
        free(frame_stack);
        frame_stack = next;
        if (frame_stack)
            set_function_args(frame_stack->args, &frame_stack->num_args);
        else
            var_assignement_word("#", "0");
    }
}
