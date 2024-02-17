#define _POSIX_C_SOURCE 200809L

#include "dot.h"

#include <stdlib.h>
#include <string.h>

#include "../executor/executor.h"
#include "../functions/functions.h"
#include "../lexer/lexer.h"
#include "../parser/parser.h"

static int has_slash(char *s)
{
    for (size_t i = 0; s[i] != '\0'; i++)
    {
        if (s[i] == '/')
            return 1;
    }
    return 0;
}

static char **separate_path(char *path)
{
    int capacity = 4;
    int i = 0;
    char **separated_paths = malloc(capacity * sizeof(char *));
    char *saveptr = NULL;
    char *p = strtok_r(path, ":", &saveptr);
    while (p != NULL)
    {
        separated_paths[i] = p;
        i++;
        if (i == capacity)
        {
            capacity *= 2;
            separated_paths =
                realloc(separated_paths, capacity * sizeof(char *));
        }
        p = strtok_r(NULL, ":", &saveptr);
    }
    separated_paths[i] = NULL;
    return separated_paths;
}

static FILE *get_file(char *filename)
{
    FILE *file = NULL;
    if (has_slash(filename))
    {
        file = fopen(filename, "r");
    }
    else
    {
        char *path = getenv("PATH");
        if (path == NULL)
            return NULL;
        char **paths = separate_path(path);
        for (int i = 0; file == NULL && paths[i] != NULL; i++)
        {
            size_t reallen = strlen(paths[i]);
            char *realfilename =
                malloc((reallen + 2 + strlen(filename)) * sizeof(char));
            strcpy(realfilename, paths[i]);
            if (filename[0] != '/')
            {
                realfilename[reallen] = '/';
                reallen++;
            }
            realfilename[reallen] = '\0';
            strcat(realfilename, filename);
            file = fopen(realfilename, "r");
            free(realfilename);
        }
        free(paths);
    }
    return file;
}

int do_command(FILE *f)
{
    struct lexer current_lexer = lexer_get();
    lexer_init(f);

    struct ast *ast = NULL;
    enum parser_status res = PARSER_OK;
    int last_ret_code = 0;
    while (res != PARSER_OK_FINISH && res != PARSER_UNEXPECTED_FINISH)
    {
        res = parse(&ast);
        if (res != PARSER_OK && res != PARSER_OK_FINISH)
        {
            last_ret_code = 2;
        }
        else if (ast != NULL)
        {
            last_ret_code = execute_ast(ast);
            free_to_be_freed();
            ast_free(ast);
        }
    }

    lexer_set(current_lexer);
    return last_ret_code;
}

int dot(char **args)
{
    if (args == NULL || args[0] == NULL)
    {
        fprintf(stderr, "./42sh: .: filename required\n");
        return 2; // No test for too many arguments: accepts
    }
    char *filename = args[0];
    FILE *file = get_file(filename);
    if (file == NULL)
    {
        fprintf(stderr, "./42sh: .: %s: No such file or directory\n", filename);
        return 1;
    }
    int res = do_command(file);
    fclose(file);
    return res;
}
