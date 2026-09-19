#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "ast.h"
#include "eval.h"

int
jcm_lex(FILE *input, struct JCMToken **tokens, int *count);

static void
parse_source(const char *source, struct JCMToken **tokens, int *token_count, struct JCMAst **ast)
{
    FILE *input;

    input = tmpfile();
    assert(input != NULL);
    assert(fwrite(source, 1, strlen(source), input) == strlen(source));
    rewind(input);
    assert(jcm_lex(input, tokens, token_count));
    fclose(input);
    assert(jcm_ast_parse(*tokens, *token_count, ast));
}

void
expect_program_result(const char *source, long expected)
{
    struct JCMToken *tokens;
    int token_count;
    struct JCMAst *ast;
    struct JCMRuntime runtime;
    struct JCMValue result;

    parse_source(source, &tokens, &token_count, &ast);
    assert(jcm_runtime_init(&runtime));
    assert(jcm_eval_program(&runtime, ast, &result));
    assert(result.kind == JCM_VALUE_NUMBER);
    assert(result.value.number == expected);
    jcm_runtime_free(&runtime);
    jcm_ast_free(ast);
    jcm_tokens_free(tokens, token_count);
}

void
expect_program_failure(const char *source)
{
    struct JCMToken *tokens;
    int token_count;
    struct JCMAst *ast;
    struct JCMRuntime runtime;
    struct JCMValue result;

    parse_source(source, &tokens, &token_count, &ast);
    assert(jcm_runtime_init(&runtime));
    assert(!jcm_eval_program(&runtime, ast, &result));
    jcm_runtime_free(&runtime);
    jcm_ast_free(ast);
    jcm_tokens_free(tokens, token_count);
}

void
expect_input_result(const char *source, const char *input_text, long expected)
{
    int saved_stdin;
    FILE *input;

    saved_stdin = dup(STDIN_FILENO);
    assert(saved_stdin >= 0);
    input = tmpfile();
    assert(input != NULL);
    assert(fwrite(input_text, 1, strlen(input_text), input) == strlen(input_text));
    rewind(input);
    assert(dup2(fileno(input), STDIN_FILENO) >= 0);
    fclose(input);
    expect_program_result(source, expected);
    assert(dup2(saved_stdin, STDIN_FILENO) >= 0);
    close(saved_stdin);
}

void
expect_output_result(const char *source, long expected, const char *output)
{
    int saved_stdout;
    FILE *capture;
    char buffer[256];
    size_t length;

    saved_stdout = dup(STDOUT_FILENO);
    assert(saved_stdout >= 0);
    capture = tmpfile();
    assert(capture != NULL);
    assert(dup2(fileno(capture), STDOUT_FILENO) >= 0);
    expect_program_result(source, expected);
    fflush(stdout);
    assert(dup2(saved_stdout, STDOUT_FILENO) >= 0);
    close(saved_stdout);
    rewind(capture);
    length = fread(buffer, 1, sizeof(buffer) - 1, capture);
    buffer[length] = '\0';
    assert(strcmp(buffer, output) == 0);
    fclose(capture);
}
