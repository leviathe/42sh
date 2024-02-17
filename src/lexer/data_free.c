#define _POSIX_C_SOURCE 200809L

#include "data_free.h"

#include <string.h>

#include "../linked_list/list.h"

static struct arguments *data_list;

void data_list_init(void)
{
    data_list = arguments_new();
}

void data_list_add(struct list *s)
{
    arguments_add(data_list, s);
}

void data_list_destroy(void)
{
    arguments_clear(data_list);
    arguments_free(data_list);
}

struct token lexer_peek2(void)
{
    return lexer_peek();
}

struct token lexer_pop2(void)
{
    struct token tok = lexer_pop();
    if (tok.type == TOKEN_TOKEN)
        data_list_add(tok.data.words);

    return tok;
}
