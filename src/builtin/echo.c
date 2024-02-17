#include "echo.h"

#include <stdio.h>
#include <string.h>

#define TRUE 1
#define FALSE 0

static inline void echo_handle_arg(char **arg, int *trailing_newline,
                                   int *interpretation)
{
    for (size_t i = 0; arg[i] && *(arg[i]) == '-'; i++)
        for (size_t j = 1; j < strlen(arg[i]); j++)
            switch (arg[i][j])
            {
            case 'n':
                *trailing_newline = FALSE;
                break;
            case 'e':
                *interpretation = TRUE;
                break;
            case 'E':
                *interpretation = FALSE;
                break;
            default:
                return;
            }
}

static inline void echo_word(const char *str, int interpretation)
{
    if (!interpretation)
    {
        fputs(str, stdout);
        return;
    }

    for (size_t i = 0; str[i]; i++)
    {
        if (str[i] == '\\')
            switch (str[i + 1])
            {
            case 'n':
                putchar('\n');
                i++;
                break;
            case 't':
                putchar('\t');
                i++;
                break;
            case '\\':
                putchar('\\');
                i++;
                break;
            default:
                putchar(str[i]);
                break;
            }
        else
            putchar(str[i]);
    }
}

void echo(char **arg)
{
    size_t j = 0;
    for (; arg[j]; j++)
    {
        // printf("arg :%sn\n", arg[j]);
    }
    int trail_new = TRUE;
    int inter = FALSE;
    echo_handle_arg(arg, &trail_new, &inter);
    size_t i = 0;
    for (; arg[i] && *(arg[i]) == '-'; i++)
    {
        char c = arg[i][1];
        if (c != 'n' && c != 'E' && c != 'e')
            break;
    }

    int first = 0;
    if (arg[i] && *(arg[i]))
    {
        echo_word(arg[i], inter);
        i++;
        first = 1;
    }

    for (; arg[i]; i++)
    {
        if ((arg[i][0] != '\0'))
        {
            if (first)
                putchar(' ');
            first = 1;
            echo_word(arg[i], inter);
        }
    }

    if (trail_new)
        putchar('\n');

    fflush(stdout);
}
