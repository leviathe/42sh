#ifndef TOKEN_H
#define TOKEN_H

#include "../linked_list/list.h"

// List all type of token
enum token_type
{
    TOKEN_OPERATOR,
    TOKEN_IO_NUMBER,
    TOKEN_TOKEN,
    TOKEN_NEWLINE,
    TOKEN_EOF,
    TOKEN_ERROR,
    TOKEN_EMPTY,
};

// List all type of operators
enum operator_type
{
    OPERATOR_PIPE,
    OPERATOR_OR_IF,
    OPERATOR_AND,
    OPERATOR_AND_IF,
    OPERATOR_SEMI,
    OPERATOR_DSEMI,
    OPERATOR_LESS,
    OPERATOR_LESSGREAT,
    OPERATOR_LESSAND,
    OPERATOR_DLESS,
    OPERATOR_DLESSDASH,
    OPERATOR_GREAT,
    OPERATOR_DGREAT,
    OPERATOR_GREATAND,
    OPERATOR_CLOBBER,
    OPERATOR_LPARENTHESIS,
    OPERATOR_RPARENTHESIS,
};

union tok_data
{
    enum operator_type op_type;
    int value;
    struct list *words;
    char *error;
};

struct token
{
    enum token_type type;
    union tok_data data;
};

#endif /* !TOKEN_H */
