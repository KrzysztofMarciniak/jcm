#ifndef JCM_AST_H
#define JCM_AST_H

#include <stdio.h>

#include "core.h"

enum JCMTokenKind {
    JCM_TOKEN_EOF,
    JCM_TOKEN_LPAREN,
    JCM_TOKEN_RPAREN,
    JCM_TOKEN_NUMBER,
    JCM_TOKEN_STRING,
    JCM_TOKEN_SYMBOL
};

struct JCMToken {
    enum JCMTokenKind kind;
    long long number;
    char *text;
    int line;
    int column;
};

enum JCMAstKind {
    JCM_AST_NUMBER,
    JCM_AST_STRING,
    JCM_AST_SYMBOL,
    JCM_AST_CORE,
    JCM_AST_LIST,
    JCM_AST_PROGRAM
};

struct JCMAst {
    enum JCMAstKind kind;

    int line;
    int column;

    union {
        long long number;
        char *string;
        char *symbol;
        enum JCMOp op;

        struct {
            struct JCMAst **items;
            int count;
        } list;

        struct {
            struct JCMAst **items;
            int count;
        } program;
    } value;
};

int
jcm_ast_parse(
    const struct JCMToken *tokens,
    int token_count,
    struct JCMAst **ast
);

void
jcm_ast_free(struct JCMAst *ast);

void
jcm_ast_print(const struct JCMAst *ast, int indent);

void
jcm_tokens_free(struct JCMToken *tokens, int count);

void
jcm_lex_free(struct JCMToken *tokens, int count);

#endif
