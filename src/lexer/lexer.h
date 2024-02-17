#ifndef LEXER_H
#define LEXER_H

#include <stdio.h>

#include "../linked_list/list.h"
#include "token.h"

struct lexer
{
    // File stream of the input
    FILE *fs;
    // Last character read
    int current_char;
    // Current_char is already used or not
    int used;
    // Boolean for the first character
    int first;
    // Lastest token consumed
    struct token current_tok;
    // Quoting context
    enum context quoting;
    // Type of expand
    enum expansion_type expand;
};

// Initialize a lexer
void lexer_init(FILE *fs);

// Get the current lexer in order to save it
struct lexer lexer_get(void);

// Sets the lexer to the argument
void lexer_set(struct lexer lexer_s);

// Set the "first" field of the lexer at TRUE
void lexer_first_word(void);

// Destroy a lexer
void lexer_destroy(void);

// Get the next token of the lexer without consuming the token
struct token lexer_peek(void);

// Get the next token and consume the token
struct token lexer_pop(void);

#endif /* !LEXER_H */
