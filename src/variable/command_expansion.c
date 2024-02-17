#define _POSIX_C_SOURCE 200809L

#include "command_expansion.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wait.h>

#include "../builtin/dot.h"

#define INIT_BUFF_SIZE 64
#define READ_BUFF_SIZE 64

static int return_error(char *s)
{
    fprintf(stderr, "./42sh: substitution: %s\n", s);
    return 1;
}

static void remove_end_newlines(char *s, int size)
{
    while (size > 0 && s[size - 1] == '\n')
        size--;
    if (size >= 0)
        s[size] = '\0';
}

/*static void remove_all_newlines(char *s)
{
    for (int i = 0; s[i] != '\0'; i++)
    {
        if (s[i] == '\n')
            s[i] = ' ';
    }
}*/

int substitute_command(struct list *list)
{
    char *s = list->word.data;

    int pipe_fd[2] = { 0, 0 };
    if (pipe(pipe_fd))
    {
        return_error("Unable to create pipe");
        return 1;
    }
    int pid = fork();
    if (pid < 0)
        return return_error("Unable to fork");
    else if (pid == 0) // Child
    {
        close(pipe_fd[0]);
        dup2(pipe_fd[1], STDOUT_FILENO); // Binding write to stdout
        FILE *f = fmemopen(s, strlen(s), "r");
        int ret = do_command(f);
        fclose(f);
        close(pipe_fd[1]);
        exit(ret);
    }
    else
    {
        free(list->word.data);
        close(pipe_fd[1]);

        size_t num_chars = 0;
        size_t size = 0;
        FILE *string_file = open_memstream(&(list->word.data), &size);
        char *buff = malloc((READ_BUFF_SIZE + 1) * sizeof(char));
        int num_read = 0;
        while ((num_read = read(pipe_fd[0], buff, READ_BUFF_SIZE)) > 0)
        {
            buff[READ_BUFF_SIZE] = '\0';
            fprintf(string_file, "%s", buff);
            num_chars += num_read;
        }
        fprintf(string_file, "%c", '\0');
        fflush(string_file);
        fclose(string_file);
        remove_end_newlines(list->word.data, num_chars - 1);
        // remove_all_newlines(list->word.data);
        free(buff);
        int status = 0;
        waitpid(pid, &status, 0);
        close(pipe_fd[0]);
        return WEXITSTATUS(status);
    }
    return 0; // Never reached
}
