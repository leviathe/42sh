#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <string.h>

#include "ast/ast.h"
#include "backend/backend.h"
#include "builtin/alias.h"
#include "executor/executor.h"
#include "functions/functions.h"
#include "lexer/data_free.h"
#include "lexer/lexer.h"
#include "parser/parser.h"
#include "variable/variable.h"

int main(int argc, char **argv)
{
    FILE *stream = get_stream(argc, argv);
    struct ast *ast = NULL;
    enum parser_status res = PARSER_OK;

    // INIT
    list_var_init();
    data_list_init();
    lexer_init(stream);
    int last_ret_code = 0;
    while (res != PARSER_OK_FINISH && res != PARSER_UNEXPECTED_FINISH)
    {
        res = parse(&ast);
        if (res != PARSER_OK && res != PARSER_OK_FINISH)
        {
            last_ret_code = 2;
            // logger_error("fail to parse\n");
        }
        else if (ast != NULL)
        {
            last_ret_code = execute_ast(ast);
            free_to_be_freed();
            ast_free(ast);
        }
    }

    // FREE
    free_functions();
    list_var_free();
    data_list_destroy();
    free_functions();
    free_aliases();
    close_stream(stream);

    return last_ret_code;
}
