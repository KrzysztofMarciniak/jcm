#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ast.h"
#include "eval.h"

/* Lexer interface supplied by lex.c. */
int
jcm_lex(FILE *input, struct JCMToken **tokens, int *count);

void
jcm_lex_free(struct JCMToken *tokens, int count);

/* Native backend interface supplied by codegen.c. */
int
jcm_codegen(FILE *out, const struct JCMAst *program);

static FILE *
open_input(const char *path)
{
    FILE *file;

    if (path == NULL || strcmp(path, "-") == 0)
        return stdin;

    file = fopen(path, "r");
    if (file == NULL)
        fprintf(stderr, "jcm: cannot open '%s'\n", path);

    return file;
}

static void
usage(const char *program)
{
    fprintf(stderr, "usage: %s [--eval | --ast | --asm] [file]\n", program);
}

/*
 * The output form already writes its value to stdout.  Do not print the
 * returned value a second time from main:
 *
 *     (↓ 'Hello')
 *
 * evaluates to the string "Hello", but ↓ has already printed it.
 */
static int
is_output_expression(const struct JCMAst *ast)
{
    return ast != NULL &&
           ast->kind == JCM_AST_LIST &&
           ast->value.list.count > 0 &&
           ast->value.list.items[0]->kind == JCM_AST_CORE &&
           ast->value.list.items[0]->value.op == JCM_OUTPUT;
}

int
main(int argc, char **argv)
{
    const char *input_path;
    const char *mode;
    FILE *input;
    struct JCMToken *tokens;
    int token_count;
    struct JCMAst *ast;
    struct JCMRuntime runtime;
    struct JCMValue result;
    int i;
    int ok;

    input_path = "-";
    mode = "--eval";

    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--eval") == 0) {
            mode = "--eval";
            continue;
        }
        if (strcmp(argv[i], "--ast") == 0) {
            mode = "--ast";
            continue;
        }
        if (strcmp(argv[i], "--asm") == 0) {
            mode = "--asm";
            continue;
        }
        if (argv[i][0] == '-' || strcmp(input_path, "-") != 0) {
            usage(argv[0]);
            return 2;
        }
        input_path = argv[i];
    }

    input = open_input(input_path);
    if (input == NULL)
        return 1;

    tokens = NULL;
    token_count = 0;
    ok = jcm_lex(input, &tokens, &token_count);
    if (input != stdin)
        fclose(input);

    if (!ok) {
        fprintf(stderr, "jcm: lexical error\n");
        return 1;
    }

    ast = NULL;
    ok = jcm_ast_parse(tokens, token_count, &ast);
    if (!ok) {
        fprintf(stderr, "jcm: syntax error\n");
        jcm_lex_free(tokens, token_count);
        return 1;
    }

    if (strcmp(mode, "--ast") == 0) {
        jcm_ast_print(ast, 0);
        jcm_ast_free(ast);
        jcm_lex_free(tokens, token_count);
        return 0;
    }

    if (strcmp(mode, "--asm") == 0) {
        ok = jcm_codegen(stdout, ast);
        jcm_ast_free(ast);
        jcm_lex_free(tokens, token_count);
        return ok ? 0 : 1;
    }

    if (!jcm_runtime_init(&runtime)) {
        fprintf(stderr, "jcm: cannot initialize runtime\n");
        jcm_ast_free(ast);
        jcm_lex_free(tokens, token_count);
        return 1;
    }

    ok = jcm_eval_program(&runtime, ast, &result);

    /* ↓ owns printing its argument; main must not print it again. */
    if (ok &&
        (ast->value.list.count == 0 ||
         !is_output_expression(ast->value.list.items[ast->value.list.count - 1])))
        jcm_value_print(result);

    jcm_runtime_free(&runtime);
    jcm_ast_free(ast);
    jcm_lex_free(tokens, token_count);
    return ok ? 0 : 1;
}
