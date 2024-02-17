#ifndef PARSER_H
#define PARSER_H

#include "../ast/ast.h"
#include "../lexer/lexer.h"

enum parser_status
{
    PARSER_OK,
    PARSER_UNEXPECTED_TOKEN,
    PARSER_UNEXPECTED_FIRST,
    PARSER_OK_FINISH,
    PARSER_UNEXPECTED_FINISH
};

enum parser_status parse(struct ast **res);

#endif /* !PARSER_H */
