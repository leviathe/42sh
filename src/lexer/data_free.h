#ifndef DATA_FREE_H
#define DATA_FREE_H

#include "../ast/arguments.h"
#include "lexer.h"

void data_list_init(void);
void data_list_add(struct list *s);
void data_list_destroy(void);
struct token lexer_peek2(void);
struct token lexer_pop2(void);

#endif /* !DATA_FREE_H */
