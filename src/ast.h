#ifndef JCM_AST_H
#define JCM_AST_H

#include <stdio.h>

#include "core.h"

/*
 * Lexer token declarations.
 *
 * These must match src/lex.c.
 */

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

/*
 * AST node kinds.
 */

enum JCMAstKind {
    JCM_AST_NUMBER,
    JCM_AST_SYMBOL,
    JCM_AST_CORE,
    JCM_AST_LIST,
    JCM_AST_PROGRAM
};

/*
 * AST node.
 *
 * LIST nodes contain child expressions.
 *
 * PROGRAM nodes contain top-level expressions.
 *
 * CORE nodes represent a recognized JCM core operator.
 */

struct JCMAst {
    enum JCMAstKind kind;

    int line;
    int column;

    union {
        long long number;

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

/*
 * Parse a complete token stream.
 *
 * Returns 1 on success.
 * Returns 0 on syntax error.
 *
 * The returned AST belongs to the caller and must be released with
 * jcm_ast_free().
 */
int
jcm_ast_parse(
    const struct JCMToken *tokens,
    int token_count,
    struct JCMAst **ast
);

/*
 * Release an AST.
 */
void
jcm_ast_free(struct JCMAst *ast);

/*
 * Print an AST for debugging.
 */
void
jcm_ast_print(const struct JCMAst *ast, int indent);


 /*
  * Release a token array.
  */
 void
 jcm_tokens_free(struct JCMToken *tokens, int count);

/*
 * Backward-compatible alias used by the CLI.
 */
void
jcm_lex_free(struct JCMToken *tokens, int count);

#endif
