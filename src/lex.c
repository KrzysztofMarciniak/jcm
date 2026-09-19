#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ast.h"

struct JCMLexer {
    FILE *input;
    int line;
    int column;
    int current;
};

static char *
jcm_strdup(const char *s)
{
    size_t n;
    char *p;

    n = strlen(s);
    p = (char *)malloc(n + 1);
    if (p == NULL)
        return NULL;

    memcpy(p, s, n + 1);
    return p;
}

static int
is_space_char(int c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' ||
           c == '\f' || c == '\v';
}

static int
is_ascii_digit(int c)
{
    return c >= '0' && c <= '9';
}

static int
is_symbol_delimiter(int c)
{
    return c == EOF || c == '(' || c == ')' || c == ';' ||
           c == '\'' || is_space_char(c);
}

static void
lexer_advance(struct JCMLexer *lexer)
{
    if (lexer->current == '\n') {
        lexer->line++;
        lexer->column = 1;
    } else {
        lexer->column++;
    }

    lexer->current = fgetc(lexer->input);
}

static void
token_init(struct JCMToken *token)
{
    token->kind = JCM_TOKEN_EOF;
    token->number = 0;
    token->text = NULL;
    token->line = 0;
    token->column = 0;
}

static void
skip_space_and_comments(struct JCMLexer *lexer)
{
    int done;

    done = 0;
    while (!done) {
        done = 1;
        while (is_space_char(lexer->current))
            lexer_advance(lexer);

        if (lexer->current == JCM_COMMENT_CHAR) {
            done = 0;
            while (lexer->current != EOF && lexer->current != '\n')
                lexer_advance(lexer);
        }
    }
}

static int
read_number(struct JCMLexer *lexer, struct JCMToken *token)
{
    char buffer[64];
    int length;
    int negative;
    unsigned long long value;
    unsigned long long limit;

    length = 0;
    negative = 0;
    value = 0;

    if (lexer->current == '-') {
        negative = 1;
        buffer[length++] = '-';
        lexer_advance(lexer);
    } else if (lexer->current == '+') {
        buffer[length++] = '+';
        lexer_advance(lexer);
    }

    if (!is_ascii_digit(lexer->current))
        return 0;

    limit = negative ? 9223372036854775808ULL : 9223372036854775807ULL;
    while (is_ascii_digit(lexer->current)) {
        unsigned int digit;

        digit = (unsigned int)(lexer->current - '0');
        if (value > (limit - digit) / 10ULL)
            return 0;

        value = value * 10ULL + digit;
        if (length < (int)sizeof(buffer) - 1)
            buffer[length++] = (char)lexer->current;
        lexer_advance(lexer);
    }

    if (!is_symbol_delimiter(lexer->current))
        return 0;

    buffer[length] = '\0';
    if (negative && value == 9223372036854775808ULL)
        token->number = (-9223372036854775807LL - 1LL);
    else if (negative)
        token->number = -(long long)value;
    else
        token->number = (long long)value;

    token->kind = JCM_TOKEN_NUMBER;
    token->text = jcm_strdup(buffer);
    return token->text != NULL;
}

static int
string_append(char **buffer, size_t *capacity, size_t *length, int c)
{
    char *new_buffer;

    if (*length + 1 >= *capacity) {
        *capacity *= 2;
        new_buffer = (char *)realloc(*buffer, *capacity);
        if (new_buffer == NULL)
            return 0;
        *buffer = new_buffer;
    }

    (*buffer)[(*length)++] = (char)c;
    (*buffer)[*length] = '\0';
    return 1;
}

/* Read a single-quoted UTF-8 byte string. */
static int
read_string(struct JCMLexer *lexer, struct JCMToken *token)
{
    char *buffer;
    size_t capacity;
    size_t length;
    int escaped;
    int c;

    capacity = 32;
    length = 0;
    escaped = 0;
    buffer = (char *)malloc(capacity);
    if (buffer == NULL)
        return 0;
    buffer[0] = '\0';

    lexer_advance(lexer); /* opening quote */
    while (lexer->current != EOF) {
        c = lexer->current;
        if (!escaped && c == '\'') {
            lexer_advance(lexer);
            token->kind = JCM_TOKEN_STRING;
            token->text = buffer;
            return 1;
        }
        if (!escaped && c == '\n') {
            free(buffer);
            return 0;
        }

        lexer_advance(lexer);
        if (escaped) {
            escaped = 0;
            if (c == 'n') c = '\n';
            else if (c == 'r') c = '\r';
            else if (c == 't') c = '\t';
            else if (c == '\\') c = '\\';
            else if (c == '\'') c = '\'';
        } else if (c == '\\') {
            escaped = 1;
            continue;
        }

        if (!string_append(&buffer, &capacity, &length, c)) {
            free(buffer);
            return 0;
        }
    }

    free(buffer);
    return 0;
}

static int
read_symbol(struct JCMLexer *lexer, struct JCMToken *token)
{
    char *buffer;
    size_t capacity;
    size_t length;

    capacity = 32;
    length = 0;
    buffer = (char *)malloc(capacity);
    if (buffer == NULL)
        return 0;

    while (!is_symbol_delimiter(lexer->current)) {
        if (!string_append(&buffer, &capacity, &length, lexer->current)) {
            free(buffer);
            return 0;
        }
        lexer_advance(lexer);
    }

    if (length == 0) {
        free(buffer);
        return 0;
    }

    token->kind = JCM_TOKEN_SYMBOL;
    token->text = buffer;
    return 1;
}

static int
lex_one(struct JCMLexer *lexer, struct JCMToken *token)
{
    token_init(token);
    skip_space_and_comments(lexer);
    token->line = lexer->line;
    token->column = lexer->column;

    if (lexer->current == EOF)
        return 1;

    if (lexer->current == '(') {
        token->kind = JCM_TOKEN_LPAREN;
        lexer_advance(lexer);
        return 1;
    }

    if (lexer->current == ')') {
        token->kind = JCM_TOKEN_RPAREN;
        lexer_advance(lexer);
        return 1;
    }

    if (lexer->current == '\'')
        return read_string(lexer, token);

    if (is_ascii_digit(lexer->current) ||
        ((lexer->current == '-' || lexer->current == '+') &&
         is_ascii_digit((c = fgetc(lexer->input))))) {
        if (c != EOF)
            ungetc(c, lexer->input);
        return read_number(lexer, token);
    }

    return read_symbol(lexer, token);
}

int
jcm_lex(FILE *input, struct JCMToken **tokens, int *count)
{
    struct JCMLexer lexer;
    struct JCMToken *result;
    int capacity;
    int length;

    if (input == NULL || tokens == NULL || count == NULL)
        return 0;

    lexer.input = input;
    lexer.line = 1;
    lexer.column = 1;
    lexer.current = fgetc(input);
    capacity = 32;
    length = 0;

    result = (struct JCMToken *)malloc(sizeof(struct JCMToken) * (size_t)capacity);
    if (result == NULL)
        return 0;

    for (;;) {
        struct JCMToken token;
        int i;

        if (!lex_one(&lexer, &token)) {
            for (i = 0; i < length; i++)
                free(result[i].text);
            free(result);
            return 0;
        }

        if (length >= capacity) {
            struct JCMToken *new_result;
            capacity *= 2;
            new_result = (struct JCMToken *)realloc(
                result, sizeof(struct JCMToken) * (size_t)capacity
            );
            if (new_result == NULL) {
                free(token.text);
                for (i = 0; i < length; i++)
                    free(result[i].text);
                free(result);
                return 0;
            }
            result = new_result;
        }

        result[length++] = token;
        if (token.kind == JCM_TOKEN_EOF)
            break;
    }

    *tokens = result;
    *count = length;
    return 1;
}

void
jcm_tokens_free(struct JCMToken *tokens, int count)
{
    int i;

    if (tokens == NULL)
        return;
    for (i = 0; i < count; i++)
        free(tokens[i].text);
    free(tokens);
}

void
jcm_lex_free(struct JCMToken *tokens, int count)
{
    jcm_tokens_free(tokens, count);
}
