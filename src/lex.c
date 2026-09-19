#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "core.h"

/* ------------------------------------------------------------------------- */
/* Token types                                                               */
/* ------------------------------------------------------------------------- */

enum JCMTokenKind {
    JCM_TOKEN_EOF,
    JCM_TOKEN_LPAREN,
    JCM_TOKEN_RPAREN,
    JCM_TOKEN_NUMBER,
    JCM_TOKEN_SYMBOL
};

struct JCMToken {
    enum JCMTokenKind kind;
    long long number;
    char *text;
    int line;
    int column;
};

/* ------------------------------------------------------------------------- */
/* Lexer                                                                      */
/* ------------------------------------------------------------------------- */

struct JCMLexer {
    FILE *input;
    int line;
    int column;
    int current;
};

/* ------------------------------------------------------------------------- */
/* Memory helpers                                                             */
/* ------------------------------------------------------------------------- */

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

/* ------------------------------------------------------------------------- */
/* Character handling                                                         */
/* ------------------------------------------------------------------------- */

static int
is_space_char(int c)
{
    return c == ' ' ||
           c == '\t' ||
           c == '\n' ||
           c == '\r' ||
           c == '\f' ||
           c == '\v';
}

static int
is_ascii_digit(int c)
{
    return c >= '0' && c <= '9';
}

static int
is_symbol_delimiter(int c)
{
    return c == EOF ||
           c == '(' ||
           c == ')' ||
           c == ';' ||
           is_space_char(c);
}

/*
 * JCM core operators containing non-ASCII characters are UTF-8.
 *
 * We don't try to interpret arbitrary UTF-8 here. Symbols are simply
 * preserved as UTF-8 byte strings.
 */

/* ------------------------------------------------------------------------- */
/* Input                                                                       */
/* ------------------------------------------------------------------------- */

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

/* ------------------------------------------------------------------------- */
/* Token initialization                                                       */
/* ------------------------------------------------------------------------- */

static void
token_init(struct JCMToken *token)
{
    token->kind = JCM_TOKEN_EOF;
    token->number = 0;
    token->text = NULL;
    token->line = 0;
    token->column = 0;
}

/* ------------------------------------------------------------------------- */
/* Comments and whitespace                                                    */
/* ------------------------------------------------------------------------- */

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

            while (lexer->current != EOF &&
                   lexer->current != '\n') {
                lexer_advance(lexer);
            }
        }
    }
}

/* ------------------------------------------------------------------------- */
/* Number parsing                                                             */
/* ------------------------------------------------------------------------- */

static int
read_number(struct JCMLexer *lexer, struct JCMToken *token)
{
    char buffer[64];
    int length;
    int negative;
    int c;
    unsigned long long value;
    unsigned long long limit;

    length = 0;
    negative = 0;
    value = 0;

    c = lexer->current;

    if (c == '-') {
        negative = 1;

        if (length < (int)sizeof(buffer) - 1)
            buffer[length++] = (char)c;

        lexer_advance(lexer);

        if (!is_ascii_digit(lexer->current))
            return 0;
    }

    if (lexer->current == '+') {
        if (length < (int)sizeof(buffer) - 1)
            buffer[length++] = '+';

        lexer_advance(lexer);

        if (!is_ascii_digit(lexer->current))
            return 0;
    }

    if (negative)
        limit = 9223372036854775808ULL;
    else
        limit = 9223372036854775807ULL;

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

    /*
     * Convert manually rather than using strtoll(), because C89 does not
     * define strtoll().
     */
    if (negative) {
        if (value == 9223372036854775808ULL)
            token->number = (-9223372036854775807LL - 1LL);
        else
            token->number = -(long long)value;
    } else {
        token->number = (long long)value;
    }

    token->kind = JCM_TOKEN_NUMBER;
    token->text = jcm_strdup(buffer);

    if (token->text == NULL)
        return 0;

    return 1;
}

/* ------------------------------------------------------------------------- */
/* Symbol parsing                                                             */
/* ------------------------------------------------------------------------- */

static int
read_symbol(struct JCMLexer *lexer, struct JCMToken *token)
{
    char *buffer;
    size_t capacity;
    size_t length;
    int c;

    capacity = 32;
    length = 0;

    buffer = (char *)malloc(capacity);
    if (buffer == NULL)
        return 0;

    while (!is_symbol_delimiter(lexer->current)) {
        c = lexer->current;

        if (length + 1 >= capacity) {
            char *new_buffer;

            capacity *= 2;

            new_buffer = (char *)realloc(buffer, capacity);

            if (new_buffer == NULL) {
                free(buffer);
                return 0;
            }

            buffer = new_buffer;
        }

        buffer[length++] = (char)c;
        lexer_advance(lexer);
    }

    if (length == 0) {
        free(buffer);
        return 0;
    }

    buffer[length] = '\0';

    token->kind = JCM_TOKEN_SYMBOL;
    token->text = buffer;

    return 1;
}

/* ------------------------------------------------------------------------- */
/* Tokenization                                                               */
/* ------------------------------------------------------------------------- */

static int
lex_one(struct JCMLexer *lexer, struct JCMToken *token)
{
    int start_line;
    int start_column;

    token_init(token);

    skip_space_and_comments(lexer);

    start_line = lexer->line;
    start_column = lexer->column;

    token->line = start_line;
    token->column = start_column;

    if (lexer->current == EOF) {
        token->kind = JCM_TOKEN_EOF;
        return 1;
    }

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

    /*
     * A number starts with a digit or with '-'/'+' followed by a digit.
     */
    if (is_ascii_digit(lexer->current)) {
        return read_number(lexer, token);
    }

    if ((lexer->current == '-' || lexer->current == '+')) {
        int sign;

        sign = lexer->current;
        lexer_advance(lexer);

        if (is_ascii_digit(lexer->current)) {
            /*
             * Put the sign back through a small temporary token path.
             * We cannot ungetc reliably for UTF-8-aware processing, so
             * construct the number directly here.
             */
            char buffer[64];
            int length;
            unsigned long long value;
            unsigned long long limit;

            length = 0;
            value = 0;

            buffer[length++] = (char)sign;

            limit = (sign == '-')
                ? 9223372036854775808ULL
                : 9223372036854775807ULL;

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

            if (sign == '-') {
                if (value == 9223372036854775808ULL)
                    token->number = (-9223372036854775807LL - 1LL);
                else
                    token->number = -(long long)value;
            } else {
                token->number = (long long)value;
            }

            token->kind = JCM_TOKEN_NUMBER;
            token->text = jcm_strdup(buffer);

            return token->text != NULL;
        }

        /*
         * '+' and '-' are also valid symbols.
         *
         * Reconstruct the symbol beginning with the sign.
         */
        {
            char *buffer;

            buffer = (char *)malloc(2);

            if (buffer == NULL)
                return 0;

            buffer[0] = (char)sign;
            buffer[1] = '\0';

            token->kind = JCM_TOKEN_SYMBOL;
            token->text = buffer;

            return 1;
        }
    }

    return read_symbol(lexer, token);
}

/* ------------------------------------------------------------------------- */
/* Public lexer API                                                           */
/* ------------------------------------------------------------------------- */

/*
 * Lex the complete input stream.
 *
 * On success:
 *
 *     return 1
 *     *tokens points to allocated token array
 *     *count contains number of tokens, including EOF
 *
 * On failure:
 *
 *     return 0
 */
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

    result = (struct JCMToken *)malloc(
        sizeof(struct JCMToken) * (size_t)capacity
    );

    if (result == NULL)
        return 0;

    for (;;) {
        struct JCMToken token;

        if (!lex_one(&lexer, &token)) {
            int i;

            for (i = 0; i < length; i++)
                free(result[i].text);

            free(result);
            return 0;
        }

        if (length >= capacity) {
            struct JCMToken *new_result;

            capacity *= 2;

            new_result = (struct JCMToken *)realloc(
                result,
                sizeof(struct JCMToken) * (size_t)capacity
            );

            if (new_result == NULL) {
                int i;

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

/* ------------------------------------------------------------------------- */
/* Token cleanup                                                              */
/* ------------------------------------------------------------------------- */

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
