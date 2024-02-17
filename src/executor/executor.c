#define _POSIX_C_SOURCE 200809L

#include "executor.h"

#include <ctype.h>
#include <fcntl.h>
#include <fnmatch.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "../builtin/alias.h"
#include "../builtin/cd.h"
#include "../builtin/dot.h"
#include "../builtin/echo.h"
#include "../builtin/export.h"
#include "../builtin/unset.h"
#include "../functions/functions.h"
#include "../lexer/lexer.h"
#include "../variable/funargs.h"
#include "../variable/variable.h"

#define SRC_FD2 STDERR_FILENO

// stop = 0 -> keep running
// stop = 1 -> continue
// stop = 2 -> break
static int stop = 0;
static int loop_descriptor = 0;
static int nesting_level = 0;

static int execute_assignement(struct ast *ast);
static int execute_exported_variables(struct ast *ast);
static int is_special_builtin(const char *s);

static int return_error(const char *s)
{
    fprintf(stderr, "%s", s);
    return 1;
}

static int get_last_ret(void)
{
    struct variable *v = get_var(strdup("?"));
    if (!v)
        return 42;
    return atoi(v->data);
}

static int our_atoi(char *s)
{
    int res = 0;
    int i = 0;
    while (s[i] != '\0' && s[i] >= '0' && s[i] <= '9')
    {
        res *= 10;
        res += s[i] - '0';
        i++;
    }
    return s[i] == '\0' ? res : -1;
}

static int our_exit(char *arg)
{
    int code = arg == NULL ? get_last_ret() : our_atoi(arg);
    if (code == -1)
    {
        fprintf(stderr, "./42sh: exit: %s: numeric argument required\n", arg);
        code = 2;
    }
    // fprintf(stderr, "exit\n");
    _exit(code);
    return code;
}

static int assign_variables(struct ast *ast)
{
    if (ast == NULL)
        return 0;

    int res = assign_variables(ast->left); // The other variables are left.
    // TODO do smth with error
    res = execute_assignement(ast);
    return res;
}

static int execute_function(char **args)
{
    struct ast *fun_ast = get_fun_ast(args[0]);
    if (fun_ast == NULL)
        return -1;
    nesting_level++;
    push_args(args);
    int res = execute_ast(fun_ast);
    pop_args();
    nesting_level--;
    return res;
}

static int is_int(char *str)
{
    if (str == NULL || *str == '\0')
        return 0;
    int i = 0;
    if (str[i] == '-')
        i++;
    for (; str[i] != '\0'; i++)
    {
        if (!isdigit(str[i]))
            return 0;
    }
    return 1;
}

int break_(char **args)
{
    if (((args[0] != NULL) && !is_int(args[0]))
        || (args[0] != NULL && args[1] != NULL))
    {
        stop = loop_descriptor;
        loop_descriptor = 0;
        if (!is_int(args[0]))
        {
            return_error("break: numeric argument required\n");
            return 128;
        }
        return_error("break: too much argument\n");
        return 1;
    }
    if (loop_descriptor < 0)
    {
        return_error("42sh: break: only meaningful in a `for', `while', or "
                     "`until' loop\n");
        return 0;
    }
    if (args[0] != NULL)
    {
        int res = atoi(args[0]);
        if (res <= 0)
        {
            stop = loop_descriptor;
            loop_descriptor = 0;
            return_error("break: loop out of range \n");
            return 1;
        }
        if (res > loop_descriptor)
            res = loop_descriptor;
        loop_descriptor -= res;
        stop = res;
    }
    else
    {
        loop_descriptor--;
        stop = 1;
    }
    return 0;
}

static int b_continue = 0;

int continue_(char **args)
{
    if (((args[0] != NULL) && !is_int(args[0]))
        || (args[0] != NULL && args[1] != NULL))
    {
        stop = loop_descriptor;
        loop_descriptor = 0;
        if (!is_int(args[0]))
        {
            return_error("continue: numeric argument required\n");
            return 128;
        }
        return_error("continue: too much argument\n");
        return 1;
    }
    if (loop_descriptor < 0)
    {
        return_error("42sh: continue: only meaningful in a `for', `while', or "
                     "`until' loop\n");
        return 0;
    }
    if (args[0] != NULL)
    {
        int res = atoi(args[0]);
        if (res <= 0)
        {
            stop = loop_descriptor;
            loop_descriptor = 0;
            return_error("continue: loop out of range \n");
            return 1;
        }
        b_continue = 1;
        res--;
        if (res > loop_descriptor - 1)
            res = loop_descriptor - 1;
        loop_descriptor -= res;
        stop = res;
    }
    else
    {
        b_continue = 1;
    }
    return 0;
}

static int execute_builtin(char **args)
{
    // printf("test : %s\n", ast->data.argument->args[0]);
    // char **args = arguments_expend(ast->data.argument->args);
    // char **args = arguments_expend(ast->data.argument->args);
    int ret_code = -1;
    if (!strcmp(args[0], "echo"))
    {
        echo(args + 1);
        ret_code = 0;
    }
    else if (!strcmp(args[0], "unset"))
    {
        unset(args + 1);
        ret_code = 0;
    }
    else if (!strcmp(args[0], "exit"))
        ret_code = our_exit(args[1]);
    else if (!strcmp(args[0], "true"))
        ret_code = 0;
    else if (!strcmp(args[0], "false"))
        ret_code = 1;
    else if (!strcmp(args[0], "."))
        ret_code = dot(args + 1);
    else if (!strcmp(args[0], "alias"))
        ret_code = alias(args + 1);
    else if (!strcmp(args[0], "unalias"))
        ret_code = unalias(args + 1);
    else if (!strcmp(args[0], "break"))
    {
        int ret = break_(args + 1);
        ret_code = ret;
    }
    else if (!strcmp(args[0], "continue"))
    {
        int ret = continue_(args + 1);
        ret_code = ret;
    }
    else if (!strcmp(args[0], "cd"))
        ret_code = cd(args + 1);
    else if (!strcmp(args[0], "export"))
        ret_code = exportv(args[1]);

    if (ret_code > -1)
        args_free(args);
    return ret_code;
}

static int free_args_and_return(char **args, int ret_code)
{
    args_free(args);
    return ret_code;
}

static int execute_command(struct ast *ast)
{
    char **args = arguments_expend(ast->data.argument->args);

    // if (args[0][0] == '\0')
    //     return free_args_and_return(args, get_last_ret()); // Empty command

    int res = execute_function(args);
    if (res > -1)
        return free_args_and_return(args, res);

    execute_exported_variables(ast->left);
    if (is_special_builtin(args[0]))
        assign_variables(ast->left);
    res = execute_builtin(args);
    if (res >= 0)
        return res;

    int pid = fork();
    if (pid == -1) // Error from Fork
        return -1;
    var_update_pid();
    if (pid == 0) // Child
    {
        // Program name and arguments
        // char **args = arguments_expend(ast->data.argument->args);
        execvp(args[0], args);
        fprintf(stderr, "./42sh: Unknown command: %s\n", args[0]);
        lexer_destroy();
        args_free(args);
        exit(127); // TODO check if necesary
    }
    else // Parent
    {
        args_free(args);
        int status = 0;
        waitpid(pid, &status, 0);
        return WEXITSTATUS(status);
    }
    return 0;
}

static int execute_redir(struct ast *ast, int src_fd, int dst_fd)
{
    int save_fd = dup(src_fd); // Saving dest
    fcntl(save_fd, F_SETFD, FD_CLOEXEC);

    int new_fd = dup2(dst_fd, src_fd);

    int res = execute_ast(ast->left);

    dup2(dst_fd, new_fd);

    dup2(save_fd, src_fd); // Restoring dest

    close(save_fd);
    return res;
}

static int execute_redir_in(struct ast *ast, char *file, int dst_fd, int flags)
{
    int arg_fd = open(file, flags | O_RDONLY);
    if (arg_fd < 0)
    {
        fprintf(stderr, "./42sh: %s: No such file or directory\n", file);
        return 1;
    }
    int res = execute_redir(ast, dst_fd, arg_fd);
    close(arg_fd);
    return res;
}

static int execute_redir_out(struct ast *ast, int src_fd, char *file, int flags)
{
    int arg_fd = open(file, flags | O_WRONLY | O_CREAT,
                      S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (arg_fd < 0)
    {
        fprintf(stderr, "./42sh: %s: Unable to create file\n", file);
        return 1;
    }
    int res = execute_redir(ast, src_fd, arg_fd);
    close(arg_fd);
    return res;
}

static int execute_redir_inout(struct ast *ast, int src_fd, char *file)
{
    int arg_fd =
        open(file, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (arg_fd < 0)
    {
        fprintf(stderr, "./42sh: %s: Unable to create file\n", file);
        return 1;
    }
    int res = execute_redir(ast, src_fd, arg_fd);
    close(arg_fd);
    return res;
}

static int execute_redir_andout(struct ast *ast, int src_fd, char *word)
{
    int dst_fd = atoi(word); // TODO handle error better
    if (dst_fd != 0 || word[0] == '0')
    {
        int fd_data = fcntl(dst_fd, F_GETFD); // word is a valid number
        if (fd_data < 0)
        {
            fprintf(stderr, "./42sh: Bad file descriptor: %d\n", dst_fd);
            return 1;
        }
        return execute_redir(ast, src_fd, dst_fd);
    }
    if (!strcmp(word, "-"))
    {
        close(src_fd);
        return execute_ast(ast->left);
    }
    return return_error("./42sh: Ambiguous redirect\n");
}

static int execute_redir_general(struct ast *ast)
{
    char **args = arguments_expend(ast->data.argument->args);
    char *arg = args[0];
    int io_num_out = STDOUT_FILENO;
    int io_num_in = STDIN_FILENO;
    if (ast->right->left)
    {
        io_num_out = ast->right->left->data.io_number;
        io_num_in = io_num_out;
        if (strlen(args[0]) >= 4 && io_num_out >= 1024)
        {
            fprintf(stderr, "./42sh: Bad file descriptor: %d\n", io_num_out);
            return free_args_and_return(args, 1);
        }
    }

    int ret = 0;
    switch (ast->right->data.redir_oper)
    {
    case OPERATOR_LESS:
        ret = execute_redir_in(ast, arg, STDIN_FILENO, 0);
        break;
    case OPERATOR_GREAT:
        ret = execute_redir_out(ast, io_num_out, arg, O_TRUNC);
        break;
    case OPERATOR_CLOBBER:
        ret = execute_redir_out(ast, io_num_out, arg, O_TRUNC);
        break;
    case OPERATOR_DGREAT:
        ret = execute_redir_out(ast, io_num_out, arg, O_APPEND);
        break;
    case OPERATOR_GREATAND:
        ret = execute_redir_andout(ast, io_num_out, arg);
        break;
    case OPERATOR_LESSAND:
        ret = execute_redir_andout(ast, io_num_in, arg);
        break;
    case OPERATOR_LESSGREAT:
        ret = execute_redir_inout(ast, io_num_in, arg);
        break;
    default:
        args_free(args);
        return execute_ast(ast->left); // TODO handle all operators here
    }
    args_free(args);
    return ret;
}

static int execute_if(struct ast *ast)
{
    int ret = execute_ast(ast->data.ast_arg);
    if (ret == 0)
        return execute_ast(ast->left);
    return execute_ast(ast->right);
}

static int execute_command_list(struct ast *ast)
{
    int ret = execute_ast(ast->left);
    if (stop)
        return ret;
    if (b_continue)
    {
        b_continue = 0;
        return ret;
    }
    if (ast->right == NULL)
        return ret;
    return execute_ast(ast->right);
}

static int execute_pipe(struct ast *ast)
{
    int pipe_fd[2] = { 0, 0 };
    int err = pipe(pipe_fd);
    if (err)
        return return_error("./42sh: pipe: Couldn't create pipe fds\n");

    int pid = fork();
    if (pid == -1)
        return return_error("./42sh: pipe: Couldn't fork processes\n");
    var_update_pid();
    if (pid == 0) // Child
    {
        close(pipe_fd[0]);
        dup2(pipe_fd[1], STDOUT_FILENO); // Binding write to stdout
        int ret_code = execute_ast(ast->left);
        lexer_destroy();
        exit(ret_code);
    }
    else // Parent
    {
        close(pipe_fd[1]);
        // TODO maybe save STDIN here and STDOUT up above
        int stdin_save = dup(STDIN_FILENO);
        int newfd =
            dup2(pipe_fd[0], STDIN_FILENO); // Binding read pipe to stdin
        int res = execute_ast(ast->right);

        int status = 0;
        waitpid(pid, &status, 0);

        close(newfd);
        dup2(stdin_save, STDIN_FILENO); // Restoring stdin
        return res;
    }

    return 127;
}

static int execute_negation(struct ast *ast)
{
    int ret = execute_ast(ast->left);
    if (ret == 0)
        return 1;
    return 0;
}

static int execute_while(struct ast *ast)
{
    int last_return = 0;
    loop_descriptor++;
    int ld = loop_descriptor;
    while (loop_descriptor >= ld && execute_ast(ast->data.ast_arg) == 0
           && loop_descriptor >= ld)
    {
        last_return = execute_ast(ast->left);
    }
    if (loop_descriptor >= ld)
        loop_descriptor--;
    if (stop)
        stop--;
    return last_return;
}

static int execute_until(struct ast *ast)
{
    int last_return = 0;
    loop_descriptor++;
    int ld = loop_descriptor;
    while (loop_descriptor >= ld && execute_ast(ast->data.ast_arg) != 0
           && loop_descriptor >= ld)
    {
        last_return = execute_ast(ast->left);
    }
    if (loop_descriptor >= ld)
        loop_descriptor--;
    if (stop)
        stop--;
    return last_return;
}

static int execute_for(struct ast *ast)
{
    loop_descriptor++;
    int ld = loop_descriptor;

    char **args = arguments_expend(ast->data.argument->args);
    int last_return = 0;
    for (int i = 1; loop_descriptor >= ld && args[i] != NULL; i++)
    {
        var_assignement_word(args[0], args[i]);
        last_return = execute_ast(ast->left);
    }
    if (loop_descriptor >= ld)
        loop_descriptor--;
    if (stop)
        stop--;

    args_free(args);
    return last_return;
}

static int execute_and_if(struct ast *ast)
{
    int ret = execute_ast(ast->left);
    if (ret != 0)
        return 1; // maybe return ret
    ret = execute_ast(ast->right);
    if (ret != 0)
        return 1; // maybe return ret
    return 0;
}

static int execute_or_if(struct ast *ast)
{
    int ret = execute_ast(ast->left);
    if (ret == 0)
        return 0; // maybe return ret
    ret = execute_ast(ast->right);
    if (ret == 0)
        return 0; // maybe return ret
    return 1;
}

static int execute_assignement(struct ast *ast)
{
    char **args = arguments_expend(ast->data.argument->args);
    char *name = args[0];
    char *data = strstr(args[0], "=");
    if (data == NULL)
    {
        args_free(args);
        return 0;
    }
    data[0] = '\0';
    data++;
    int size = 1;
    size += strlen(data);
    for (size_t i = 1; args[i]; i++)
        size += strlen(args[i]) + 2;
    char *value = calloc(1, size);
    int b = 0;
    if (data[0] != '\0')
    {
        strcat(value, data);
        b = 1;
    }
    for (size_t i = 1; args[i]; i++)
    {
        if (b)
            strcat(value, " ");
        b = 1;
        strcat(value, args[i]);
    }
    int ret = var_assignement_word(name, value);
    free(value);
    args_free(args);
    return ret;
}

static int execute_subshell(struct ast *ast)
{
    int pid = fork();
    if (pid == -1)
        return return_error("./42sh: subshell: Couldn't fork processes\n");
    var_update_pid();
    if (pid == 0) // Child
    {
        _exit(execute_ast(ast));
    }
    int status;
    waitpid(pid, &status, 0);

    return WEXITSTATUS(status);
}

static int is_special_builtin(const char *s)
{
    return !strcmp(s, "break") || !strcmp(s, "continue") || !strcmp(s, ".")
        || !strcmp(s, "exit") || !strcmp(s, "export");
}

/*
** Executes the definition of a function.
** If a function of the same name already exists, it is overwritten.
** One cannot overwrite a special builtin; regular builtins and programs work.
*/
static int execute_function_definition(struct ast *ast)
{
    // No expansion must be done to the name of the function
    char *name = ast->data.argument->args[0]->word.data;
    if (is_special_builtin(name))
    {
        fprintf(stderr, "./42sh: '%s': is a special shell builtin\n", name);
        return 2;
    }
    add_fun(name, ast->left, nesting_level);
    // ast->left = NULL; // Once this function is recognized, we have to remove
    // its AST from the parsed AST, because it would be freed
    // at each whole AST free
    return 0;
}

/*
** To be executed on the left child of a AST_COMMAND, which contains chained
** ASTs representing to-be-exported BUT NOT SET variables.
** They are chained left-child wise.
*/
static int execute_exported_variables(struct ast *ast)
{
    if (ast == NULL)
        return 0;

    execute_exported_variables(ast->left);

    char **args = arguments_expend(ast->data.argument->args);
    char *name = args[0];
    char *data = strstr(args[0], "=");
    if (data == NULL)
    {
        args_free(args);
        return 0;
    }
    data[0] = '\0';
    data++;
    int size = 1;
    size += strlen(data);
    for (size_t i = 1; args[i]; i++)
        size += strlen(args[i]) + 2;
    char *value = calloc(1, size);
    int b = 0;
    if (data[0] != '\0')
    {
        strcat(value, data);
        b = 1;
    }
    for (size_t i = 1; args[i]; i++)
    {
        if (b)
            strcat(value, " ");
        b = 1;
        strcat(value, args[i]);
    }

    setenv(name, value, 1); // The 1 is for overwriting.
    free(value);
    args_free(args);
    return 0;
}

static char *case_construct_single_string(char **args, int use_ifs)
{
    char *res = NULL;
    size_t size = 0;
    FILE *f = open_memstream(&res, &size);
    fprintf(f, "%s", args[0]);
    struct variable *ifs = get_var(strdup("IFS"));
    char ifs_c = ifs == NULL ? ' ' : ifs->name[0];
    for (int i = 1; args[i] != NULL; i++)
    {
        if (use_ifs)
            fprintf(f, "%c%s", ifs_c, args[i]);
        else
            fprintf(f, "%s", args[i]);
    }
    fprintf(f, "%c", '\0');
    fclose(f);
    return res;
}

static int case_successful_match(struct ast *ast)
{
    if (ast == NULL)
        return 0;

    if (ast->type != AST_CASE_ITEM)
        return execute_ast(ast);

    return case_successful_match(ast->left);
}

static char *add_char(char *s, char c, int *size, int *capacity)
{
    s[*size] = c;
    *size += 1;
    if (*size == *capacity)
    {
        *capacity *= 2;
        s = realloc(s, sizeof(char *) * *capacity);
    }
    return s;
}

static char *case_escape_special_chars_string(char *s, enum context c)
{
    int capacity = 8;
    int size = 0;
    char *res = malloc(capacity * sizeof(char));

    for (size_t i = 0; s[i] != '\0'; i++)
    {
        if ((s[i] == '*' || s[i] == '?') && c != CONTEXT_UNQUOTED)
            add_char(res, '\\', &size, &capacity);
        res = add_char(res, s[i], &size, &capacity);
        // is_escape = s[i] == '\\' && !is_escape;
    }
    res[size] = '\0';

    return res;
}

static void case_escape_special_chars(struct list **args)
{
    for (size_t i = 0; args[i] != NULL; i++)
    {
        for (struct list *l = args[i]; l != NULL; l = l->next)
        {
            char *copy =
                case_escape_special_chars_string(l->word.data, l->word.context);
            free(l->word.data);
            l->word.data = copy;
        }
    }
}

/*
** The argument is an AST_CASE_ITEM. This function tries to match the argument
** and all its left children to the given string.
** If successful, executes the associated functions and return the associated
** code; if not, returns -1.
*/
static int case_match_item(struct ast *ast, char *s)
{
    if (ast == NULL || ast->type != AST_CASE_ITEM)
        return -1;

    struct arguments *args_copy = arguments_copy(ast->data.argument);
    case_escape_special_chars(args_copy->args);
    char **args = arguments_expend(args_copy->args);
    arguments_clear(args_copy);
    arguments_free(args_copy);
    char *p = case_construct_single_string(args, 0);
    args_free(args);
    int res = -1;
    if (!fnmatch(p, s, 0))
    {
        res = case_successful_match(ast->left);
    }
    else
    {
        res = case_match_item(ast->left, s);
    }
    free(p);
    return res;
}

/*
** Executes the switch case. The word to switch is in the arguments.
** The left child of the node is a right-childrened list of AST_CASE_LIST;
** each of these nodes have as argument an ast representing this node, and as
** left child a left-childrened list of AST_ITEM, ended potentially by a
** compound_list. An AST_ITEM contains as argument the pattern to match with.
*/
static int execute_case(struct ast *ast)
{
    char **args = arguments_expend(ast->data.argument->args);
    char *s = case_construct_single_string(args, 1);
    args_free(args);

    struct ast *case_list = ast->left;
    int res = -1;
    while (case_list != NULL && res < 0)
    {
        res = case_match_item(case_list->left, s);
        case_list = case_list->right;
    }
    free(s);
    return res != -1 ? res : 0; // Default return code is 0
}

/*
** Secondary function of execute_ast. Basically all the switch cases that
** couldn't fit in the original function.
*/
static int execute_ast_secondary_switch(struct ast *ast)
{
    int ret = -1;
    switch (ast->type)
    {
    case AST_FOR:
        ret = execute_for(ast);
        break;
    case AST_ASSIGNEMENT:
        ret = assign_variables(ast); // Recursively assign variables
        break;
    case AST_LONE_ASSIGNMENT:
        ret = execute_ast(ast->left); // No command -> assign variables
        break;
    case AST_SUBSHELL:
        ret = execute_subshell(ast->left); // No command -> assign variables
        break;
    case AST_FUNCTION:
        ret = execute_function_definition(ast);
        break;
    case AST_CASE:
        ret = execute_case(ast);
        break;
    default:
        ret = -1;
    }
    return ret;
}

int execute_ast(struct ast *ast)
{
    if (ast == NULL)
        return 0;

    int ret = -1;
    switch (ast->type)
    {
    case AST_COMMAND_LIST:
        ret = execute_command_list(ast);
        break;
    case AST_COMMAND:
        ret = execute_command(ast);
        break;
    case AST_IF:
        ret = execute_if(ast);
        break;
    case AST_REDIR:
        ret = execute_redir_general(ast);
        break;
    case AST_PIPE:
        ret = execute_pipe(ast); // TODO change fonction to pipe
        break;
    case AST_NEGATION:
        ret = execute_negation(ast);
        break;
    case AST_WHILE:
        ret = execute_while(ast);
        break;
    case AST_UNTIL:
        ret = execute_until(ast);
        break;
    case AST_AND_IF:
        ret = execute_and_if(ast);
        break;
    case AST_OR_IF:
        ret = execute_or_if(ast);
        break;
    default:
        ret = execute_ast_secondary_switch(ast);
    }
    var_update_exit_code(ret); // actualyse var
    return ret;
}
