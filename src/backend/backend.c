#define _POSIX_C_SOURCE 200809L

#include <err.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

FILE *get_stream(int argc, char **argv)
{
    FILE *stream = NULL;
    // No argument
    if (argc == 1)
        return stdin;

    if (strcmp(argv[1], "-c") == 0)
    {
        if (argc < 3)
            errx(
                1,
                "42sh: -c: option requires an argument\n42sh: usage: [OPTIONS] "
                "[SCRIPT] [ARGUMENTS ...]\n");

        stream = fmemopen(argv[2], strlen(argv[2]), "r");
    }
    else
        stream = fopen(argv[1], "r");

    if (stream == NULL)
        errx(1,
             "42sh: unable to open stream\n42sh: usage [OPTIONS] [SCRIPT] "
             "[ARGUMENTS ...]\n");

    return stream;
}

void close_stream(FILE *stream)
{
    if (stream != stdin && fclose(stream) != 0)
        errx(1, "42sh: unable to close the stream");
}
