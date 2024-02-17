#define _POSIX_C_SOURCE 200809L
#include "lexer.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "../backend/backend.h"
#include "../builtin/alias.h"
#include "../vector/vector.h"

#define TRUE 1
#define FALSE 0
#define LEXCHAR lexer.current_char

typedef struct token (*tokenizer)(void);
typedef struct token (*word_function)(struct vector *v, struct list *words);

static inline struct token token_recognition(void);
static inline struct token double_quoted_word(struct vector *v,
                                              struct list *words);
static inline struct token single_quoted_word(struct vector *v,
                                              struct list *words);
static inline struct token unquoted_word(struct vector *v, struct list *words);

static FILE *main_fs;

static struct lexer lexer = {
    .fs = NULL,
    .used = TRUE,
    .first = TRUE,
    .current_tok.type = TOKEN_EMPTY,
    .quoting = CONTEXT_UNQUOTED,
    .expand = EXP_NONE,
};

static const word_function quoting[3] = {
    [CONTEXT_UNQUOTED] = unquoted_word,
    [CONTEXT_SQUOTED] = single_quoted_word,
    [CONTEXT_DQUOTED] = double_quoted_word,
};

// Initialize the lexer
void lexer_init(FILE *fs)
{
    main_fs = fs;
    lexer.fs = fs;
    lexer.used = TRUE;
    lexer.first = TRUE;
    lexer.current_tok.type = TOKEN_EMPTY;
    lexer.quoting = CONTEXT_UNQUOTED;
    lexer.expand = EXP_NONE;
}

struct lexer lexer_get(void)
{
    return lexer;
}

void lexer_set(struct lexer lexer_s)
{
    lexer = lexer_s;
}

void lexer_destroy(void)
{
    close_stream(lexer.fs);
}

void lexer_first_word(void)
{
    lexer.first = TRUE;
}

// Skip all content of a comment
static inline struct token skip_comment(void)
{
    int c = fgetc(lexer.fs);
    while (c != '\n' && c != EOF)
        c = fgetc(lexer.fs);

    LEXCHAR = c;
    lexer.used = TRUE;
    lexer.current_tok.type = c == '\n' ? TOKEN_NEWLINE : TOKEN_EOF;

    return lexer.current_tok;
}

/**********     OPERATORS     **********/
// Manage the escape newline case
static inline struct token test_escape(enum operator_type op_type,
                                       tokenizer ftok)
{
    if ((LEXCHAR = fgetc(lexer.fs)) == '\n')
        return ftok();

    ungetc(LEXCHAR, lexer.fs);
    LEXCHAR = '\\';
    lexer.current_tok.data.op_type = op_type;
    lexer.used = FALSE;
    return lexer.current_tok;
}

// Manage pipe operator and its derivations
static inline struct token pipe_operator(void)
{
    lexer.used = TRUE;
    lexer.current_tok.type = TOKEN_OPERATOR;
    switch ((LEXCHAR = fgetc(lexer.fs)))
    {
    case '\\':
        return test_escape(OPERATOR_PIPE, pipe_operator);
    case '|':
        lexer.current_tok.data.op_type = OPERATOR_OR_IF;
        break;
    default:
        lexer.current_tok.data.op_type = OPERATOR_PIPE;
        lexer.used = FALSE;
        break;
    }

    return lexer.current_tok;
}

// Manage "and" operator and derivations
static inline struct token and_operator(void)
{
    lexer.used = TRUE;
    lexer.current_tok.type = TOKEN_OPERATOR;
    switch ((LEXCHAR = fgetc(lexer.fs)))
    {
    case '&':
        lexer.current_tok.data.op_type = OPERATOR_AND_IF;
        break;
    case '\\':
        return test_escape(OPERATOR_AND, and_operator);
    default:
        lexer.current_tok.data.op_type = OPERATOR_AND;
        lexer.used = FALSE;
        break;
    }

    return lexer.current_tok;
}

// Manage "semiclon" operator and derivations
static inline struct token semicolon_operator(void)
{
    lexer.used = TRUE;
    lexer.current_tok.type = TOKEN_OPERATOR;
    switch ((LEXCHAR = fgetc(lexer.fs)))
    {
    case ';':
        lexer.current_tok.data.op_type = OPERATOR_DSEMI;
        break;
    case '\\':
        return test_escape(OPERATOR_SEMI, semicolon_operator);
    default:
        lexer.current_tok.data.op_type = OPERATOR_SEMI;
        lexer.used = FALSE;
        break;
    }

    return lexer.current_tok;
}

// Manage "double left angle bracket" operator and derivations
static inline struct token dless_operator(void)
{
    switch ((LEXCHAR = fgetc(lexer.fs)))
    {
    case '-':
        lexer.current_tok.data.op_type = OPERATOR_DLESSDASH;
        break;
    case '\\':
        return test_escape(OPERATOR_DLESS, dless_operator);
    default:
        lexer.current_tok.data.op_type = OPERATOR_DLESS;
        lexer.used = FALSE;
        break;
    }

    return lexer.current_tok;
}

// Manage "left angle bracket" operator and derivations
static inline struct token less_operator(void)
{
    lexer.used = TRUE;
    lexer.current_tok.type = TOKEN_OPERATOR;
    switch ((LEXCHAR = fgetc(lexer.fs)))
    {
    case '>':
        lexer.current_tok.data.op_type = OPERATOR_LESSGREAT;
        break;
    case '&':
        lexer.current_tok.data.op_type = OPERATOR_LESSAND;
        break;
    case '<':
        return dless_operator();
    case '\\':
        return test_escape(OPERATOR_LESS, less_operator);
    default:
        lexer.current_tok.data.op_type = OPERATOR_LESS;
        lexer.used = FALSE;
        break;
    }

    return lexer.current_tok;
}

// Manage "right angle bracket" operator and derivations
static inline struct token great_operator(void)
{
    lexer.used = TRUE;
    lexer.current_tok.type = TOKEN_OPERATOR;
    switch ((LEXCHAR = fgetc(lexer.fs)))
    {
    case '>':
        lexer.current_tok.data.op_type = OPERATOR_DGREAT;
        break;
    case '&':
        lexer.current_tok.data.op_type = OPERATOR_GREATAND;
        break;
    case '|':
        lexer.current_tok.data.op_type = OPERATOR_CLOBBER;
        break;
    case '\\':
        return test_escape(OPERATOR_GREAT, great_operator);
    default:
        lexer.current_tok.data.op_type = OPERATOR_GREAT;
        lexer.used = FALSE;
        break;
    }

    return lexer.current_tok;
}

// Manage "left parenthesis" operator
static inline struct token left_parenthesis_operator(void)
{
    lexer.used = TRUE;
    lexer.current_tok = (struct token){ .type = TOKEN_OPERATOR,
                                        .data.op_type = OPERATOR_LPARENTHESIS };
    return lexer.current_tok;
}

// Manage "right parenthesis" operator
static inline struct token right_parenthesis_operator(void)
{
    lexer.used = TRUE;
    lexer.current_tok = (struct token){ .type = TOKEN_OPERATOR,
                                        .data.op_type = OPERATOR_RPARENTHESIS };
    return lexer.current_tok;
}

/**********     WORDS     **********/
/* Managment of words:
** 1) build_word: initialize the vector that will contain the word
** 2) unquoted_word: core function, used to lex an unqoted word or to redirect
** in single_quoted or double_quoted
** 3) single_quoted | double_quoted: used to
** read between quotes and apply the corresponding rules.
** 4) end_word: return TOKEN_ERROR if the flag is set or the TOKEN_TOKEN
**/

// Check if the character "c" is an operator or a part of an operator
static inline int is_operator(char c)
{
    return c == '|' || c == '&' || c == ';' || c == '<' || c == '>' || c == '('
        || c == ')';
}

static inline int is_spe_var(char c)
{
    return (c >= '0' && c <= '9') || c == '@' || c == '?' || c == '*'
        || c == '#' || c == '$';
}

// Check if the character "c" is a delimiter of a word
static inline int is_delimiter(char c)
{
    return isspace(c) || is_operator(c) || c == '\'' || c == '"' || c == '\\'
        || c == EOF;
}

// Check if the character "c" is a potential substitution character
static inline int is_sub(char c)
{
    return c == '`' || c == '$';
}

static inline int dquote_esc_spe_char(char c)
{
    return c == '$' || c == '`' || c == '"' || c == '\\';
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

static inline struct token toktok_error(struct vector *v, struct list *words,
                                        char *msg)
{
    vector_destroy(v);
    list_destroy(words);
    return (struct token){ .type = TOKEN_ERROR, .data.error = msg };
}

static inline struct token toktok_end(struct vector *v, struct list *words)
{
    vector_destroy(v);
    if (lexer.first && words && !words->next && !is_reserved_word(words->word))
    {
        char *value = get_alias(words->word.data);
        if (!value)
            goto end;

        list_destroy(words);
        FILE *fs = fmemopen(value, strlen(value), "r");
        if (!fs)
        {
            return (struct token){ .type = TOKEN_ERROR,
                                   .data.error = "alias: fmemopen issue" };
        }

        lexer.fs = fs;
        lexer.first = FALSE;
        return token_recognition();
    }

end:
    lexer.first = FALSE;
    return (struct token){ .type = TOKEN_TOKEN, .data.words = words };
}

static inline struct token new_word(struct vector *v, struct list *words,
                                    word_function fword)
{
    if (v->size)
    {
        struct word word = { .context = lexer.quoting,
                             .exp = lexer.expand,
                             .data = strndup(v->data, v->size) };
        v->size = 0;
        if (!word.data)
            return toktok_error(v, words, "word allocation: Not enough memory");

        words = list_append(words, word);
    }

    return fword(v, words);
}

static inline struct token single_quoted_word(struct vector *v,
                                              struct list *words)
{
    lexer.quoting = CONTEXT_SQUOTED;
    lexer.expand = EXP_NONE;
    lexer.used = TRUE;
    LEXCHAR = fgetc(lexer.fs);
    for (; LEXCHAR != '\'' && LEXCHAR != EOF; LEXCHAR = fgetc(lexer.fs))
        vector_append(v, LEXCHAR);

    return (LEXCHAR == EOF) ? toktok_error(v, words,
                                           "single quoting: unexpected EOF "
                                           "while looking for matching `''")
                            : new_word(v, words, unquoted_word);
}

static inline struct token special_param(struct vector *v, struct list *words)
{
    lexer.expand = EXP_VARIABLE;
    vector_append(v, LEXCHAR);
    return new_word(v, words, quoting[lexer.quoting]);
}

static inline struct token expand_var(struct vector *v, struct list *words)
{
    lexer.expand = EXP_VARIABLE;
    for (; !is_delimiter(LEXCHAR) && !is_spe_var(LEXCHAR) && LEXCHAR != '$'
         && (lexer.quoting != CONTEXT_DQUOTED || LEXCHAR != '"');
         LEXCHAR = fgetc(lexer.fs))
        vector_append(v, LEXCHAR);

    ungetc(LEXCHAR, lexer.fs);
    return new_word(v, words, quoting[lexer.quoting]);
}

static inline struct token expand_var_brace(struct vector *v,
                                            struct list *words)
{
    lexer.expand = EXP_VARIABLE;
    LEXCHAR = fgetc(lexer.fs);
    for (; !is_delimiter(LEXCHAR) && LEXCHAR != '}'
         && (lexer.quoting != CONTEXT_DQUOTED || LEXCHAR != '"');
         LEXCHAR = fgetc(lexer.fs))
        vector_append(v, LEXCHAR);

    return (lexer.quoting == CONTEXT_DQUOTED && LEXCHAR == '"')
            || is_delimiter(LEXCHAR)
        ? toktok_error(
            v, words,
            "variable substitution: unexpected EOF while looking for "
            "matching `}'")
        : new_word(v, words, quoting[lexer.quoting]);
}

static inline struct token expand_cmd_dollar(struct vector *v,
                                             struct list *words)
{
    lexer.expand = EXP_COMMAND;
    int parenthesis = 1;
    for (;
         LEXCHAR != EOF && (lexer.quoting != CONTEXT_DQUOTED || LEXCHAR != '"');
         LEXCHAR = fgetc(lexer.fs))
    {
        if (LEXCHAR == '\\')
        {
            vector_append(v, '\\');
            LEXCHAR = fgetc(lexer.fs);
        }
        else if (LEXCHAR == '(')
            parenthesis += 1;
        else if (LEXCHAR == ')')
        {
            parenthesis -= 1;
            if (!parenthesis)
                goto end;
        }

        vector_append(v, LEXCHAR);
    }
end:
    return LEXCHAR == EOF ? toktok_error(v, words,
                                         "command substitution: unexpected EOF "
                                         "while looking for matching `)'")
                          : new_word(v, words, quoting[lexer.quoting]);
}

static inline struct token expand_cmd_backquote(struct vector *v,
                                                struct list *words)
{
    lexer.expand = EXP_COMMAND;
    LEXCHAR = fgetc(lexer.fs);
    for (; LEXCHAR != EOF && LEXCHAR != '`'; LEXCHAR = fgetc(lexer.fs))
    {
        if (LEXCHAR == '\\')
        {
            LEXCHAR = fgetc(lexer.fs);
            if (LEXCHAR != '`')
                vector_append(v, '\\');
        }

        vector_append(v, LEXCHAR);
    }

    return LEXCHAR == EOF
        ? toktok_error(v, words,
                       "command substitution: unexpected EOF while looking for "
                       "matching ``'")
        : new_word(v, words, quoting[lexer.quoting]);
}

static inline struct token expand_arith(struct vector *v, struct list *words)
{
    lexer.expand = EXP_ARITHMETIC;
    return toktok_error(v, words,
                        "arithmetic substitution: unexpected EOF while looking "
                        "for matching `))'");
}

static inline struct token expand_cmd_arith(struct vector *v,
                                            struct list *words)
{
    LEXCHAR = fgetc(lexer.fs);
    return (LEXCHAR == '(' ? expand_arith : expand_cmd_dollar)(v, words);
}

static inline struct token expand(struct vector *v, struct list *words)
{
    static word_function fexp[] = {
        ['#'] = special_param,    ['$'] = special_param,
        ['('] = expand_cmd_arith, ['*'] = special_param,
        ['?'] = special_param,    ['@'] = special_param,
        ['0'] = special_param,    ['1'] = special_param,
        ['2'] = special_param,    ['3'] = special_param,
        ['4'] = special_param,    ['5'] = special_param,
        ['6'] = special_param,    ['7'] = special_param,
        ['8'] = special_param,    ['9'] = special_param,
        ['{'] = expand_var_brace,
    };

    word_function fword = NULL;
    return LEXCHAR < '#' || LEXCHAR > '{' || !(fword = fexp[LEXCHAR])
        ? expand_var(v, words)
        : fword(v, words);
}

static inline struct token double_quoted_word(struct vector *v,
                                              struct list *words)
{
    lexer.quoting = CONTEXT_DQUOTED;
    lexer.expand = EXP_NONE;
    lexer.used = TRUE;

loop:
    LEXCHAR = fgetc(lexer.fs);
    for (; LEXCHAR != '"' && LEXCHAR != '\\' && !is_sub(LEXCHAR)
         && LEXCHAR != EOF;
         LEXCHAR = fgetc(lexer.fs))
        vector_append(v, LEXCHAR);

    switch (LEXCHAR)
    {
    case EOF:
        return toktok_error(v, words,
                            "double quoting: unexpected EOF while looking "
                            "for matching `\"'");
    case '"':
        return new_word(v, words, unquoted_word);
    case '`':
        return expand_cmd_backquote(v, words);
    case '$':
        if ((LEXCHAR = fgetc(lexer.fs)) != '"' && !isspace(LEXCHAR))
            return new_word(v, words, expand);

        vector_append(v, '$');
        if (LEXCHAR != '"')
            vector_append(v, LEXCHAR);
        else
            ungetc(LEXCHAR, lexer.fs);
        goto loop;
    default:
        switch ((LEXCHAR = fgetc(lexer.fs)))
        {
        case EOF:
            return toktok_error(v, words,
                                "double quoting: unexpected EOF while looking "
                                "for matching `\"'");
        case '\n':
            goto loop;
        default:
            if (!dquote_esc_spe_char(LEXCHAR))
                vector_append(v, '\\');
            vector_append(v, LEXCHAR);
            goto loop;
        }
    }
}

static inline struct token unquoted_word(struct vector *v, struct list *words)
{
    lexer.quoting = CONTEXT_UNQUOTED;
    lexer.expand = EXP_NONE;
    if (lexer.used)
        LEXCHAR = fgetc(lexer.fs);

    lexer.used = TRUE;
    for (;; LEXCHAR = fgetc(lexer.fs))
    {
        for (; !is_delimiter(LEXCHAR) && !is_sub(LEXCHAR);
             LEXCHAR = fgetc(lexer.fs))
            vector_append(v, LEXCHAR);

        switch (LEXCHAR)
        {
        case EOF:
            return new_word(v, words, toktok_end);
        case '\'':
            return new_word(v, words, single_quoted_word);
        case '"':
            return new_word(v, words, double_quoted_word);
        case '\\':
            if ((LEXCHAR = fgetc(lexer.fs)) == EOF)
                vector_append(v, '\\');
            else if (LEXCHAR != '\n')
                vector_append(v, LEXCHAR);
            break;
        case '`':
            return new_word(v, words, expand_cmd_backquote);
        case '$':
            if ((LEXCHAR = fgetc(lexer.fs)) != EOF && !isspace(LEXCHAR))
                return new_word(v, words, expand);

            vector_append(v, '$');
            break;
        default:
            lexer.used = FALSE;
            return new_word(v, words, toktok_end);
        }
    }
}

static inline struct token build_word(void)
{
    struct vector *v = vector_init(16);
    if (!v)
        return (struct token){ .type = TOKEN_ERROR };

    switch (LEXCHAR)
    {
    case '\'':
        return single_quoted_word(v, NULL);
    case '"':
        return double_quoted_word(v, NULL);
    default:
        return unquoted_word(v, NULL);
    }
}

/**********     IO Number     **********/

static inline int is_redir(const char c)
{
    return c == '<' || c == '>';
}

static inline struct token io_number(void)
{
    struct vector *v = vector_init(16);
    if (!v)
        return (struct token){ .type = TOKEN_ERROR };

    vector_append(v, LEXCHAR);
    LEXCHAR = fgetc(lexer.fs);
    for (; isdigit(LEXCHAR); LEXCHAR = fgetc(lexer.fs))
        vector_append(v, LEXCHAR);

    lexer.used = FALSE;
    if (v->size > 4 || !is_redir(LEXCHAR))
        return unquoted_word(v, NULL);

    int value = atoi(v->data);
    vector_destroy(v);
    return (struct token){ .type = TOKEN_IO_NUMBER, .data.value = value };
}

/**********     Recognition     **********/
static inline struct token eof_recognition(void)
{
    lexer.used = TRUE;
    return (struct token){ .type = TOKEN_EOF };
}

static inline struct token newline_recognition(void)
{
    lexer.used = TRUE;
    return (struct token){ .type = TOKEN_NEWLINE };
}

static inline struct token backslash_recognition(void)
{
    if ((LEXCHAR = fgetc(lexer.fs)) == '\n')
    {
        lexer.used = TRUE;
        return token_recognition();
    }

    ungetc(LEXCHAR, lexer.fs);
    LEXCHAR = '\\';
    return build_word();
}

// ALIAS
// Name : _ [:digit:] [:alpha:] ! % , @

// Suppose that current_token is consumed
static inline struct token token_recognition(void)
{
    static const tokenizer lex_func[] = {
        [1] = eof_recognition,
        ['\n'] = newline_recognition,
        ['#'] = skip_comment,
        ['&'] = and_operator,
        ['('] = left_parenthesis_operator,
        [')'] = right_parenthesis_operator,
        ['0'] = io_number,
        ['1'] = io_number,
        ['2'] = io_number,
        ['3'] = io_number,
        ['4'] = io_number,
        ['5'] = io_number,
        ['6'] = io_number,
        ['7'] = io_number,
        ['8'] = io_number,
        ['9'] = io_number,
        [';'] = semicolon_operator,
        ['<'] = less_operator,
        ['>'] = great_operator,
        ['\\'] = backslash_recognition,
        ['|'] = pipe_operator,
    };

    if (lexer.used)
        LEXCHAR = fgetc(lexer.fs);

    while (isblank(LEXCHAR))
        LEXCHAR = fgetc(lexer.fs);

    lexer.used = FALSE;
    tokenizer ftok = NULL;
    return LEXCHAR <= '|' && (ftok = lex_func[abs(LEXCHAR)]) ? ftok()
                                                             : build_word();
}

// Get the next token of the lexer without consuming the token
struct token lexer_peek(void)
{
    if (lexer.current_tok.type == TOKEN_EMPTY)
    {
        lexer.current_tok = token_recognition();
        if (lexer.current_tok.type == TOKEN_TOKEN
            && lexer.current_tok.data.words == NULL)
        {
            struct list *words = malloc(sizeof(struct list));
            words->word.data = strdup("");
            words->word.context = CONTEXT_UNQUOTED;
            words->word.exp = EXP_NONE;
            words->next = NULL;
            lexer.current_tok.data.words = words;
        }
    }

    return lexer.current_tok;
}

// Get the next token and consume the token
struct token lexer_pop(void)
{
    struct token tok = lexer.current_tok.type == TOKEN_EMPTY
        ? lexer_peek()
        : lexer.current_tok;
    lexer.current_tok.type = TOKEN_EMPTY;
    if (lexer.fs != main_fs)
    {
        for (LEXCHAR = fgetc(lexer.fs); isspace(LEXCHAR);
             LEXCHAR = fgetc(lexer.fs))
            continue;

        if (LEXCHAR == EOF)
        {
            fclose(lexer.fs);
            lexer.fs = main_fs;
        }
        else
            ungetc(LEXCHAR, lexer.fs);
    }

    return tok;
}
