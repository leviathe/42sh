#include "parser.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../lexer/data_free.h"

static enum parser_status parse_list(struct ast **res);
static enum parser_status parse_and_or(struct ast **res);
static enum parser_status parse_pipeline(struct ast **res);
static enum parser_status parse_command(struct ast **res);
static enum parser_status parse_shell_command(struct ast **res);
static enum parser_status parse_rule_if(struct ast **res);
static enum parser_status parse_else_clause(struct ast **res);
static enum parser_status parse_else(struct ast **res);
static enum parser_status parse_elif(struct ast **res);
static enum parser_status parse_compound_list(struct ast **res);
static enum parser_status parse_simple_command(struct ast **res);
static enum parser_status parse_simple_command_words(struct ast **res,
                                                     struct list *words);
static enum parser_status parse_element(struct ast ***res);
static enum parser_status parse_prefix(struct ast ***res);
static enum parser_status parse_redirection(struct ast ***res);
static enum parser_status parse_while(struct ast **res);
static enum parser_status parse_until(struct ast **res);
static enum parser_status parse_for(struct ast **res);
static enum parser_status parse_fundec(struct ast **res, struct list **words);
static enum parser_status parse_case(struct ast **res);
static enum parser_status parse_case_clause(struct ast **res);
static enum parser_status parse_case_item(struct ast **res);

/*
** Small function to check if a string is a valid function/variable identifier
*/
static int is_name(char *s)
{
    if (isdigit(s[0]) || !isalpha(s[0]))
        return 0;
    int i = 1;
    while (s[i] != '\0' && (isalnum(s[i]) || s[i] == '_'))
        i++;
    return s[i] == '\0';
}

static int is_reserved_word(struct word w)
{
    char *s = w.data;
    return w.context == CONTEXT_UNQUOTED
        && (!strcmp(s, "then") || !strcmp(s, "fi") || !strcmp(s, "done")
            || !strcmp(s, "do") || !strcmp(s, "elif") || !strcmp(s, "else")
            || !strcmp(s, "until") || !strcmp(s, "for") || !strcmp(s, "if")
            || !strcmp(s, "esac") || !strcmp(s, "in") || !strcmp(s, "while")
            || !strcmp(s, "!") || !strcmp(s, "{") || !strcmp(s, "}")
            || !strcmp(s, "case"));
}

static enum parser_status is_ERROR_EOF(void)
{
    struct token tok = lexer_peek2();
    while (tok.type != TOKEN_EOF && tok.type != TOKEN_NEWLINE)
    {
        lexer_pop2();
        tok = lexer_peek2();
    }
    while (tok.type == TOKEN_NEWLINE)
    {
        lexer_pop2();
        tok = lexer_peek2();
    }

    return tok.type == TOKEN_EOF ? PARSER_UNEXPECTED_FINISH
                                 : PARSER_UNEXPECTED_TOKEN;
}

static enum parser_status is_EOF(void)
{
    struct token tok = lexer_pop2();
    if (tok.type == TOKEN_EOF)
    {
        return PARSER_OK_FINISH;
    }
    return PARSER_OK;
}

static enum parser_status return_free(struct ast **a, char *msg)
{
    if (!msg)
        fprintf(stderr, "./42sh: An error occured while parsing ast\n");
    else
        fprintf(stderr, "./42sh: %s\n", msg);
    ast_free_all(*a);
    return is_ERROR_EOF();
}

static struct ast *ast_copy(struct ast *a)
{
    struct ast *b = ast_new_arg(a->type);
    arguments_free(b->data.argument);
    b->data.argument = a->data.argument;
    b->left = a->left;
    b->right = a->right;
    return b;
}

enum parser_status parse(struct ast **res)
{
    *res = NULL;
    lexer_first_word();
    struct token tok = lexer_peek2();
    if (tok.type == TOKEN_ERROR) // Just in case
        return return_free(res, NULL);
    if (tok.type == TOKEN_EOF || tok.type == TOKEN_NEWLINE
        || tok.type == TOKEN_EMPTY)
    {
        return is_EOF();
    }
    if (parse_list(res) == PARSER_UNEXPECTED_TOKEN) // Entering rule list
        return return_free(res, NULL);

    tok = lexer_peek2();
    if (tok.type == TOKEN_ERROR) // Is this needed ?
        return return_free(res, tok.data.error);
    if (tok.type == TOKEN_EOF || tok.type == TOKEN_NEWLINE
        || tok.type == TOKEN_EMPTY)
    {
        return is_EOF();
    }

    return return_free(res, NULL);
}

static enum parser_status parse_list(struct ast **res)
{
    enum parser_status ret_and_or = parse_and_or(res); // Rule and_or
    if (ret_and_or != PARSER_OK)
        return ret_and_or;
    // *res has an AST

    struct token tok = lexer_peek2(); // Semicolon
    enum parser_status and_or_status = PARSER_OK;
    while ((tok.type == TOKEN_OPERATOR && tok.data.op_type == OPERATOR_SEMI)
           && and_or_status == PARSER_OK)
    {
        (*res)->left = ast_copy(*res); // res' left child is a copy of res
        (*res)->type = AST_COMMAND_LIST;
        (*res)->data.argument = arguments_new();
        (*res)->right = NULL;
        res = &((*res)->right);
        lexer_pop2(); // Popping semicolon
        and_or_status = parse_and_or(res); // Entering and_or
        if (and_or_status == PARSER_OK) // may cause error
            tok = lexer_peek2(); // Potential semicolon
        else if (and_or_status == PARSER_UNEXPECTED_FIRST) // If first char
            return PARSER_OK; // doesn't match and_or, it is the [ ; ].
        else // Else, at least a char was read; then it is an error
            return PARSER_UNEXPECTED_TOKEN;
    }
    return PARSER_OK;
}

static enum parser_status parse_and_or(struct ast **res)
{
    enum parser_status ret_pipeline = parse_pipeline(res); // Rule and_or
    if (ret_pipeline != PARSER_OK)
        return ret_pipeline;

    struct token tok = lexer_peek2(); // search for '&&' / '||'
    while (
        (tok.type == TOKEN_OPERATOR && tok.data.op_type == OPERATOR_AND_IF)
        || (tok.type == TOKEN_OPERATOR && tok.data.op_type == OPERATOR_OR_IF))
    {
        enum operator_type op_type = tok.data.op_type;
        lexer_pop2();
        tok = lexer_peek2();
        while (tok.type == TOKEN_NEWLINE)
        {
            lexer_pop2();
            tok = lexer_peek2();
        }

        (*res)->left = ast_copy(*res); // res' left child is a copy of res
        if (op_type == OPERATOR_AND_IF)
            (*res)->type = AST_AND_IF;
        else
            (*res)->type = AST_OR_IF;
        (*res)->data.argument = arguments_new();
        (*res)->right = NULL;

        if (parse_pipeline(&((*res)->right)) != PARSER_OK)
            return PARSER_UNEXPECTED_TOKEN;

        tok = lexer_peek2();
    }

    return PARSER_OK;
}

static enum parser_status parse_pipeline(struct ast **res)
{
    // return PARSER_UNEXPECTED_TOKEN;
    struct token tok = lexer_peek2();
    enum parser_status return_status = PARSER_UNEXPECTED_FIRST;
    if (tok.type == TOKEN_TOKEN && !strcmp(tok.data.words->word.data, "!"))
    {
        lexer_pop2();
        (*res) = ast_new_ast(AST_NEGATION);
        (*res)->data.argument = arguments_new();
        res = &((*res)->left);
        return_status = PARSER_UNEXPECTED_TOKEN;
    }

    enum parser_status status = parse_command(res);
    if (status != PARSER_OK)
        return status == PARSER_UNEXPECTED_FIRST ? return_status
                                                 : PARSER_UNEXPECTED_TOKEN;

    tok = lexer_peek2();
    while (tok.type == TOKEN_OPERATOR && tok.data.op_type == OPERATOR_PIPE)
    {
        lexer_pop2();
        tok = lexer_peek2();
        while (tok.type == TOKEN_NEWLINE)
        {
            lexer_pop2();
            tok = lexer_peek2();
        }
        (*res)->left = ast_copy(*res); // res' left child is a copy of res
        (*res)->type = AST_PIPE;
        (*res)->data.argument = arguments_new();
        (*res)->right = NULL;
        res = &((*res)->right);
        if (parse_command(res) != PARSER_OK)
            return PARSER_UNEXPECTED_TOKEN;
        tok = lexer_peek2();
    }
    return PARSER_OK;
}

static enum parser_status redir_loop(struct ast ***res)
{
    enum parser_status redir_status = parse_redirection(res);
    while (redir_status == PARSER_OK)
        redir_status = parse_redirection(res);

    return redir_status != PARSER_UNEXPECTED_TOKEN ? PARSER_OK
                                                   : PARSER_UNEXPECTED_TOKEN;
}

/*static enum parser_status parse_command(struct ast **res)
{
    enum parser_status ret = parse_simple_command(res);
    if (ret != PARSER_UNEXPECTED_FIRST)
        return ret;

    ret = parse_shell_command(res);
    if (ret == PARSER_UNEXPECTED_TOKEN)
        return PARSER_UNEXPECTED_TOKEN; // Failed shell command
    else if(ret == PARSER_UNEXPECTED_FIRST && parse_fundec(res) != PARSER_OK)
        return PARSER_UNEXPECTED_TOKEN; // Failed function declaration

    return redir_loop(&res);
}*/

static enum parser_status parse_command(struct ast **res)
{
    struct list *first_word = NULL;
    enum parser_status ret = parse_fundec(res, &first_word);
    if (ret == PARSER_UNEXPECTED_TOKEN)
        return PARSER_UNEXPECTED_TOKEN; // Function failed after '('
    if (ret == PARSER_OK)
        return redir_loop(&res); // Function failed before '('

    if (first_word)
        return parse_simple_command_words(res, first_word);

    ret = parse_simple_command(res);
    if (ret != PARSER_UNEXPECTED_FIRST)
        return ret;

    ret = parse_shell_command(res); // simple_command didn't work
    if (ret != PARSER_OK)
        return ret;
    return redir_loop(&res);
}

static enum parser_status parse_simple_command_words(struct ast **res,
                                                     struct list *words)
{
    if (*res == NULL)
        *res = ast_new_arg(AST_COMMAND); // Now it has to be the second rule
    else
        (*res)->type = AST_COMMAND;
    arguments_add((*res)->data.argument, words);

    enum parser_status element_status = parse_element(&res);
    while (element_status == PARSER_OK)
        element_status = parse_element(&res);
    return element_status == PARSER_UNEXPECTED_FIRST ? PARSER_OK
                                                     : PARSER_UNEXPECTED_TOKEN;
}

static enum parser_status parse_simple_command(struct ast **res)
{
    enum parser_status prefix_status = parse_prefix(&res);
    int one_prefix = 0;
    while (prefix_status == PARSER_OK)
    {
        one_prefix = 1;
        prefix_status = parse_prefix(&res);
    }
    if (prefix_status == PARSER_UNEXPECTED_TOKEN)
        return PARSER_UNEXPECTED_TOKEN;

    struct token tok = lexer_peek2();
    if (tok.type != TOKEN_TOKEN
        || is_reserved_word(tok.data.words->word)) // Getting a WORD token
    {
        return one_prefix ? PARSER_OK : PARSER_UNEXPECTED_FIRST;
        // No token was consumed IF no prefix was detected
        // If prefix, it is the first rule: No word
    }
    tok = lexer_pop2();
    return parse_simple_command_words(res, tok.data.words);
}

static enum parser_status parse_command_block(struct ast **res)
{
    struct token tok = lexer_peek2();
    if (tok.type != TOKEN_TOKEN || strcmp(tok.data.words->word.data, "{")
        || tok.data.words->word.context != CONTEXT_UNQUOTED)
        return PARSER_UNEXPECTED_FIRST;
    lexer_pop2();

    if (parse_compound_list(res) != PARSER_OK)
        return PARSER_UNEXPECTED_TOKEN;

    tok = lexer_pop2();
    if (tok.type != TOKEN_TOKEN || strcmp(tok.data.words->word.data, "}"))
        return PARSER_UNEXPECTED_TOKEN;
    return PARSER_OK;
}

static enum parser_status parse_subshell(struct ast **res)
{
    struct token tok = lexer_peek2();
    if (tok.type != TOKEN_OPERATOR || tok.data.op_type != OPERATOR_LPARENTHESIS)
        return PARSER_UNEXPECTED_FIRST;
    lexer_pop2();
    *res = ast_new_ast(AST_SUBSHELL);
    if (parse_compound_list(&(*res)->left) != PARSER_OK)
        return PARSER_UNEXPECTED_TOKEN;

    tok = lexer_peek2();
    if (tok.type != TOKEN_OPERATOR || tok.data.op_type != OPERATOR_RPARENTHESIS)
        return PARSER_UNEXPECTED_TOKEN;
    lexer_pop2();
    return PARSER_OK;
}

/*
** shell_command =
**   '{' compound_list '}' -> Command block ^
** | '(' compound_list ')' -> subshell
** | rule_if
** | rule_while
** | rule_until
** | rule_for
*/
static enum parser_status parse_shell_command(struct ast **res)
{
    enum parser_status status = parse_subshell(res);
    if (status != PARSER_UNEXPECTED_FIRST)
        return status;
    status = parse_rule_if(res);
    if (status != PARSER_UNEXPECTED_FIRST)
        return status;
    status = parse_while(res);
    if (status != PARSER_UNEXPECTED_FIRST)
        return status;
    status = parse_until(res);
    if (status != PARSER_UNEXPECTED_FIRST)
        return status;
    status = parse_case(res);
    if (status != PARSER_UNEXPECTED_FIRST)
        return status;
    status = parse_for(res);
    if (status != PARSER_UNEXPECTED_FIRST)
        return status;
    return parse_command_block(res);
}

static enum parser_status parse_while(struct ast **res)
{
    struct token tok = lexer_peek2();
    if (tok.type != TOKEN_TOKEN || strcmp(tok.data.words->word.data, "while"))
    {
        return PARSER_UNEXPECTED_FIRST; // No while
    }
    lexer_pop2();
    *res = ast_new_ast(AST_WHILE);

    if (parse_compound_list(&((*res)->data.ast_arg)) != PARSER_OK)
        return PARSER_UNEXPECTED_TOKEN; // Rule compound_list failed
    tok = lexer_pop2(); // Getting the (potential) do
    if (tok.type != TOKEN_TOKEN || strcmp(tok.data.words->word.data, "do"))
    {
        return PARSER_UNEXPECTED_TOKEN; // No 'do' (or it was quoted: TODO)
    }
    if (parse_compound_list(&((*res)->left)) != PARSER_OK)
        return PARSER_UNEXPECTED_TOKEN; // compound_list failed
    tok = lexer_pop2();
    if (tok.type != TOKEN_TOKEN || strcmp(tok.data.words->word.data, "done"))
    {
        return PARSER_UNEXPECTED_TOKEN; // No done
    }

    return PARSER_OK;
}

static enum parser_status parse_until(struct ast **res)
{
    struct token tok = lexer_peek2();
    if (tok.type != TOKEN_TOKEN || strcmp(tok.data.words->word.data, "until"))
    {
        return PARSER_UNEXPECTED_FIRST; // No until
    }
    lexer_pop2();
    *res = ast_new_ast(AST_UNTIL);

    if (parse_compound_list(&((*res)->data.ast_arg)) != PARSER_OK)
        return PARSER_UNEXPECTED_TOKEN; // Rule compound_list failed
    tok = lexer_pop2(); // Getting the (potential) do
    if (tok.type != TOKEN_TOKEN || strcmp(tok.data.words->word.data, "do"))
    {
        return PARSER_UNEXPECTED_TOKEN; // No 'do' (or it was quoted: TODO)
    }
    if (parse_compound_list(&((*res)->left)) != PARSER_OK)
        return PARSER_UNEXPECTED_TOKEN; // compound_list failed
    tok = lexer_pop2();
    if (tok.type != TOKEN_TOKEN || strcmp(tok.data.words->word.data, "done"))
    {
        return PARSER_UNEXPECTED_TOKEN; // No done
    }

    return PARSER_OK;
}

int is_assign_word_(struct list *data)
{
    if (data->word.context != CONTEXT_UNQUOTED)
        return 0;
    char *ret = strstr(data->word.data, "=");
    return ret != NULL; // TODO verify to improve
}

enum parser_status parse_prefix(struct ast ***res)
{
    struct token tok = lexer_peek2();
    if (tok.type == TOKEN_TOKEN && is_assign_word_(tok.data.words))
    {
        lexer_pop2();

        struct ast *assw_node = ast_new_arg(AST_ASSIGNEMENT);
        assw_node->data.argument->args[0] = tok.data.words;

        if ((**res) == NULL)
            (**res) = ast_new_arg(AST_LONE_ASSIGNMENT);
        else
            assw_node->left = (**res)->left; // Joining the already there var
                                             // to the new var node
        (**res)->left = assw_node;
        // (*res) = &((**res)->left);
        return PARSER_OK;
    }
    return parse_redirection(res);
}

static int is_operator(struct token tok)
{
    return tok.type == TOKEN_OPERATOR
        && (tok.data.op_type == OPERATOR_LESS
            || tok.data.op_type == OPERATOR_LESSGREAT
            || tok.data.op_type == OPERATOR_LESSAND
            || tok.data.op_type == OPERATOR_DLESS
            || tok.data.op_type == OPERATOR_DLESSDASH
            || tok.data.op_type == OPERATOR_GREAT
            || tok.data.op_type == OPERATOR_DGREAT
            || tok.data.op_type == OPERATOR_GREATAND
            || tok.data.op_type == OPERATOR_CLOBBER);
}

static enum parser_status parse_redirection(struct ast ***res)
{
    if (res == NULL)
        return PARSER_OK; // TODO remove this condition
    enum parser_status err_status = PARSER_UNEXPECTED_FIRST;
    struct token tok = lexer_peek2();
    struct ast *io_num = NULL;
    if (tok.type == TOKEN_IO_NUMBER)
    {
        io_num = ast_new_ast(AST_REDIR_OP);
        io_num->data.io_number = tok.data.value;
        lexer_pop2();
        err_status = PARSER_UNEXPECTED_TOKEN;
    }
    tok = lexer_peek2();
    if (!is_operator(tok))
    {
        ast_free(io_num);
        return err_status;
    }

    if (**res)
    {
        (**res)->left = ast_copy(**res); // res' left child is a copy of res
        (**res)->data.argument = arguments_new();
    }
    else // in the case the redirection is prefix
        (**res) = ast_new_arg(AST_REDIR);
    (**res)->type = AST_REDIR;
    (**res)->right = ast_new_ast(AST_REDIR_OP);
    (**res)->right->data.redir_oper = tok.data.op_type;
    (**res)->right->left = io_num;
    if (io_num)
        (**res)->right->left = io_num;
    lexer_pop2(); // Popping redir symbol

    tok = lexer_peek2(); // Getting argument
    if (tok.type != TOKEN_TOKEN)
    {
        // ast_free(io_num);
        return PARSER_UNEXPECTED_TOKEN; // No argument
    }
    arguments_add((**res)->data.argument, tok.data.words);
    lexer_pop2(); // Popping argument

    *res = &((**res)->left);

    return PARSER_OK;
}

static enum parser_status parse_rule_if(struct ast **res)
{
    struct token tok = lexer_peek2();
    if (tok.type != TOKEN_TOKEN || strcmp(tok.data.words->word.data, "if"))
    {
        return PARSER_UNEXPECTED_FIRST; // No if
    }
    lexer_pop2();
    *res = ast_new_ast(AST_IF);

    if (parse_compound_list(&((*res)->data.ast_arg)) != PARSER_OK)
        return PARSER_UNEXPECTED_TOKEN; // Rule compound_list failed
    tok = lexer_pop2(); // Getting the (potential) then
    if (tok.type != TOKEN_TOKEN || strcmp(tok.data.words->word.data, "then"))
    {
        return PARSER_UNEXPECTED_TOKEN; // No then (or it was quoted: TODO)
    }
    if (parse_compound_list(&((*res)->left)) != PARSER_OK)
        return PARSER_UNEXPECTED_TOKEN; // compound_list failed
    if (parse_else_clause(&((*res)->right)) == PARSER_UNEXPECTED_TOKEN)
        return PARSER_UNEXPECTED_TOKEN; // else failed totally: no 'else'
    tok = lexer_pop2();
    if (tok.type != TOKEN_TOKEN || strcmp(tok.data.words->word.data, "fi"))
    {
        return PARSER_UNEXPECTED_TOKEN; // No fi
    }

    return PARSER_OK;
}

static enum parser_status parse_else_clause(struct ast **res)
{
    switch (parse_else(res))
    {
    case PARSER_UNEXPECTED_TOKEN:
        return PARSER_UNEXPECTED_TOKEN;
    case PARSER_OK:
        return PARSER_OK;
    default:
        return parse_elif(res); // If no else, try elif
    }
}

static enum parser_status parse_else(struct ast **res)
{
    struct token tok = lexer_peek2();
    if (tok.type != TOKEN_TOKEN || strcmp(tok.data.words->word.data, "else"))
    {
        return PARSER_UNEXPECTED_FIRST; // No else
    }
    lexer_pop2(); // Else
    if (parse_compound_list(res) != PARSER_OK)
        return PARSER_UNEXPECTED_TOKEN; // No compound_list

    return PARSER_OK;
}

static enum parser_status parse_elif(struct ast **res)
{
    struct token tok = lexer_peek2();
    if (tok.type != TOKEN_TOKEN || strcmp(tok.data.words->word.data, "elif"))
    {
        return PARSER_UNEXPECTED_FIRST; // No elif
    }
    lexer_pop2(); // elif
    *res = ast_new_ast(AST_IF);

    if (parse_compound_list(&((*res)->data.ast_arg)) != PARSER_OK)
        return PARSER_UNEXPECTED_TOKEN;
    tok = lexer_pop2(); // (possibly) then
    if (tok.type != TOKEN_TOKEN || strcmp(tok.data.words->word.data, "then"))
    {
        return PARSER_UNEXPECTED_TOKEN; // no then
    }
    if (parse_compound_list(&((*res)->left)) != PARSER_OK)
        return PARSER_UNEXPECTED_TOKEN; // compound_list failed
    if (parse_else_clause(&((*res)->right)) == PARSER_UNEXPECTED_TOKEN)
        return PARSER_UNEXPECTED_TOKEN; // else_clause failed (totally)

    return PARSER_OK;
}

static enum parser_status parse_compound_list(struct ast **res)
{
    struct token tok = lexer_peek2();
    enum parser_status to_return = PARSER_UNEXPECTED_FIRST;
    while (tok.type == TOKEN_NEWLINE)
    {
        lexer_pop2();
        tok = lexer_peek2();
        to_return = PARSER_UNEXPECTED_TOKEN; // Because pop
    }
    enum parser_status ret_and_or = parse_and_or(res);
    if (ret_and_or != PARSER_OK)
        return to_return; // may change to error

    tok = lexer_peek2();
    while (tok.type == TOKEN_NEWLINE
           || (tok.type == TOKEN_OPERATOR && tok.data.op_type == OPERATOR_SEMI))
    {
        enum token_type prev_tok = tok.type;
        lexer_pop2(); // Popping newline|semicolon
        tok = lexer_peek2();
        while (tok.type == TOKEN_NEWLINE) // Newline loop
        {
            lexer_pop2();
            tok = lexer_peek2();
        }
        (*res)->left = ast_copy(*res); // res' left child is a copy of res
        (*res)->type = AST_COMMAND_LIST;
        (*res)->data.argument = arguments_new();
        (*res)->right = NULL;
        res = &((*res)->right);

        ret_and_or = parse_and_or(res); // and_or rule
        if (ret_and_or == PARSER_UNEXPECTED_TOKEN)
            return PARSER_UNEXPECTED_TOKEN;
        else if (ret_and_or == PARSER_UNEXPECTED_FIRST)
        {
            if (prev_tok == TOKEN_NEWLINE)
                return PARSER_OK; //
            break; // If semicolon, it is the optional one after
        };
        tok = lexer_peek2();
    }
    tok = lexer_peek2();
    while (tok.type == TOKEN_NEWLINE) // Another newline loop
    {
        lexer_pop2();
        tok = lexer_peek2();
    }

    return PARSER_OK;
}

static enum parser_status newline_loop(void)
{
    struct token tok = lexer_peek2();
    enum parser_status out = PARSER_UNEXPECTED_FIRST;
    while (tok.type == TOKEN_NEWLINE)
    {
        lexer_pop2();
        tok = lexer_peek2();
        out = PARSER_OK;
    }
    return tok.type == TOKEN_ERROR ? PARSER_UNEXPECTED_TOKEN : out;
}

static enum parser_status parse_for_in_clause(struct ast **res)
{
    enum parser_status newline_res = newline_loop();
    if (newline_res == PARSER_UNEXPECTED_TOKEN)
        return PARSER_UNEXPECTED_TOKEN;

    struct token tok = lexer_peek2();
    if (tok.type != TOKEN_TOKEN || strcmp(tok.data.words->word.data, "in"))
        return PARSER_UNEXPECTED_FIRST; // Because the \ns are also after that

    lexer_pop2();
    tok = lexer_peek2();
    while (tok.type == TOKEN_TOKEN)
    {
        arguments_add((*res)->data.argument, tok.data.words);
        lexer_pop2();
        tok = lexer_peek2();
    }

    if (tok.type != TOKEN_NEWLINE
        && !(tok.type == TOKEN_OPERATOR && tok.data.op_type == OPERATOR_SEMI))
        return PARSER_UNEXPECTED_TOKEN;

    lexer_pop2();
    return PARSER_OK;
}

static enum parser_status parse_for(struct ast **res)
{
    struct token tok = lexer_peek2();
    if (tok.type != TOKEN_TOKEN || strcmp(tok.data.words->word.data, "for"))
        return PARSER_UNEXPECTED_FIRST; // No for
    lexer_pop2();

    *res = ast_new_arg(AST_FOR);

    tok = lexer_peek2();
    if (tok.type != TOKEN_TOKEN)
        return PARSER_UNEXPECTED_FIRST; // No word
    arguments_add((*res)->data.argument, tok.data.words);
    lexer_pop2();

    tok = lexer_peek2();
    if (tok.type == TOKEN_OPERATOR && tok.data.op_type == OPERATOR_SEMI)
        lexer_pop2();
    else
    {
        enum parser_status in_res = parse_for_in_clause(res);
        if (in_res == PARSER_UNEXPECTED_TOKEN)
            return PARSER_UNEXPECTED_TOKEN;
    }
    if (newline_loop() == PARSER_UNEXPECTED_TOKEN)
        return PARSER_UNEXPECTED_TOKEN;

    tok = lexer_peek2();
    if (tok.type != TOKEN_TOKEN || strcmp(tok.data.words->word.data, "do"))
        return PARSER_UNEXPECTED_TOKEN; // No do
    lexer_pop2();

    // The commands go on the left side of the AST
    if (parse_compound_list(&((*res)->left)) != PARSER_OK)
        return PARSER_UNEXPECTED_TOKEN;

    tok = lexer_peek2();
    if (tok.type != TOKEN_TOKEN || strcmp(tok.data.words->word.data, "done"))
        return PARSER_UNEXPECTED_TOKEN; // No done
    lexer_pop2();
    return PARSER_OK;
}

/*
** Temporary function to parser the () at function declaration.
** // TODO: Ask lexer to provide LPAR and RPAR operator tokens.
*/
static enum parser_status parse_function_parenthesis(void)
{
    struct token tok = lexer_peek2();
    if (tok.type != TOKEN_OPERATOR || tok.data.op_type != OPERATOR_LPARENTHESIS)
        return PARSER_UNEXPECTED_FIRST;
    lexer_pop2();
    tok = lexer_peek2();
    if (tok.type != TOKEN_OPERATOR || tok.data.op_type != OPERATOR_RPARENTHESIS)
        return PARSER_UNEXPECTED_TOKEN;
    lexer_pop2();

    return PARSER_OK;
}

static enum parser_status parse_fundec(struct ast **res, struct list **words)
{
    struct token tok = lexer_peek2();
    if (tok.type != TOKEN_TOKEN || is_reserved_word(tok.data.words->word)
        || !is_name(tok.data.words->word.data))
        return PARSER_UNEXPECTED_FIRST;
    tok = lexer_pop2();
    *words = tok.data.words;

    enum parser_status par_status = parse_function_parenthesis();
    if (par_status != PARSER_OK)
        return par_status;

    *res = ast_new_arg(AST_FUNCTION);
    arguments_add((*res)->data.argument, *words);

    newline_loop(); // TODO add AST support for functions
    return parse_shell_command(&((*res)->left)) == PARSER_OK // Command is in
        ? PARSER_OK // the left child
        : PARSER_UNEXPECTED_TOKEN;
}

static enum parser_status parse_element(struct ast ***res)
{
    struct token tok = lexer_peek2();
    if (tok.type == TOKEN_TOKEN)
    {
        tok = lexer_pop2(); // Element

        if ((**res)->type == AST_COMMAND)
            arguments_add((**res)->data.argument, tok.data.words);
        return PARSER_OK;
    }
    return parse_redirection(res);
}

/*
** rule_case = 'case' WORD {'\n'} 'in' {'\n'} [case_clause] 'esac' ;
*/
static enum parser_status parse_case(struct ast **res)
{
    struct token tok = lexer_peek2(); // case
    if (tok.type != TOKEN_TOKEN || strcmp(tok.data.words->word.data, "case")
        || tok.data.words->word.context != CONTEXT_UNQUOTED)
        return PARSER_UNEXPECTED_FIRST; // No case
    lexer_pop2();
    *res = ast_new_arg(AST_CASE);

    tok = lexer_peek2(); // WORD
    if (tok.type != TOKEN_TOKEN)
        return PARSER_UNEXPECTED_TOKEN;
    lexer_pop2();
    arguments_add((*res)->data.argument, tok.data.words);

    if (newline_loop() == PARSER_UNEXPECTED_TOKEN) // { \n }
        return PARSER_UNEXPECTED_TOKEN;

    tok = lexer_peek2();
    if (tok.type != TOKEN_TOKEN || strcmp(tok.data.words->word.data, "in")
        || tok.data.words->word.context != CONTEXT_UNQUOTED)
        return PARSER_UNEXPECTED_TOKEN; // No in
    lexer_pop2();

    if (newline_loop() == PARSER_UNEXPECTED_TOKEN) // { \n }
        return PARSER_UNEXPECTED_TOKEN;

    if (parse_case_clause(&((*res)->left)) == PARSER_UNEXPECTED_TOKEN)
        return PARSER_UNEXPECTED_TOKEN;

    tok = lexer_pop2();
    if (tok.type != TOKEN_TOKEN || strcmp(tok.data.words->word.data, "esac")
        || tok.data.words->word.context != CONTEXT_UNQUOTED)
        return PARSER_UNEXPECTED_TOKEN; // No esac
    return PARSER_OK;
}

/*
** case_clause = case_item { ';;' {'\n'} case_item } [';;'] {'\n'} ;
*/
static enum parser_status parse_case_clause(struct ast **res)
{
    *res = ast_new_ast(AST_CASE_LIST);

    enum parser_status status = parse_case_item(&((*res)->left));
    if (status != PARSER_OK)
        return status;

    while (status == PARSER_OK)
    {
        struct token tok = lexer_peek2();
        if (tok.type != TOKEN_NEWLINE
            && !(tok.type == TOKEN_OPERATOR
                 && tok.data.op_type == OPERATOR_DSEMI))
            return PARSER_OK;
        if (tok.type == TOKEN_NEWLINE) // Optional ;; is not here
            return newline_loop() != PARSER_UNEXPECTED_TOKEN
                ? PARSER_OK
                : PARSER_UNEXPECTED_TOKEN;

        lexer_pop2(); // Popping the ;;
        if (newline_loop() == PARSER_UNEXPECTED_TOKEN)
            return PARSER_UNEXPECTED_TOKEN;

        (*res)->right = ast_new_ast(AST_CASE_LIST);
        res = &((*res)->right);
        status = parse_case_item(&((*res)->left));
    }
    return status == PARSER_UNEXPECTED_FIRST ? PARSER_OK
                                             : PARSER_UNEXPECTED_TOKEN;
}

/*
** ['('] WORD { '|' WORD } ')' {'\n'} [compound_list] ;
*/
static enum parser_status parse_case_item(struct ast **res)
{
    enum parser_status status = PARSER_UNEXPECTED_FIRST;
    struct token tok = lexer_peek2();
    if (tok.type == TOKEN_OPERATOR && tok.data.op_type == OPERATOR_LPARENTHESIS)
    {
        lexer_pop2();
        tok = lexer_peek2();
        status = PARSER_UNEXPECTED_TOKEN;
    }

    if (tok.type != TOKEN_TOKEN || is_reserved_word(tok.data.words->word))
        return status;
    lexer_pop2();

    *res = ast_new_arg(AST_CASE_ITEM);
    arguments_add((*res)->data.argument, tok.data.words);

    tok = lexer_peek2();
    while (tok.type == TOKEN_OPERATOR && tok.data.op_type == OPERATOR_PIPE)
    {
        lexer_pop2();
        tok = lexer_peek2();
        if (tok.type != TOKEN_TOKEN)
            return status;
        lexer_pop2();

        (*res)->left = ast_new_arg(AST_CASE_ITEM);
        res = &((*res)->left);
        arguments_add((*res)->data.argument, tok.data.words);

        tok = lexer_peek2();
    }
    if (tok.type != TOKEN_OPERATOR || tok.data.op_type != OPERATOR_RPARENTHESIS)
        return PARSER_UNEXPECTED_TOKEN;
    lexer_pop2();

    if (newline_loop() == PARSER_UNEXPECTED_TOKEN)
        return PARSER_UNEXPECTED_TOKEN;

    return parse_compound_list(&((*res)->left)) == PARSER_UNEXPECTED_TOKEN
        ? PARSER_UNEXPECTED_TOKEN
        : PARSER_OK;
}
