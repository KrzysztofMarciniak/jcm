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

/* Copy source to output, expanding #include "filename" recursively. */
static int
expand_includes(FILE *input, FILE *output, const char *base, int depth)
{
    char line[4096];

    if (depth > 64)
        return 0;

    while (fgets(line, sizeof(line), input) != NULL) {
        char filename[2048];
        char path[4096];
        char *p;
        FILE *included;

        p = line;
        while (*p == ' ' || *p == '\t')
            p++;

        if (sscanf(p, "#include \"%2047[^\"]\"", filename) == 1 &&
            strncmp(p, "#include", 8) == 0) {
            if (base != NULL && base[0] != '\0')
                snprintf(path, sizeof(path), "%s/%s", base, filename);
            else
                snprintf(path, sizeof(path), "%s", filename);

            included = fopen(path, "r");
            if (included == NULL) {
                fprintf(stderr, "jcm: cannot open include '%s'\n", path);
                return 0;
            }
            if (!expand_includes(included, output, base, depth + 1)) {
                fclose(included);
                return 0;
            }
            fclose(included);
        } else if (fputs(line, output) == EOF) {
            return 0;
        }
    }

    return !ferror(input);
}

static int
prepare_source(FILE *input, const char *path, FILE **prepared)
{
    char base[4096];
    const char *slash;
    FILE *temp;

    temp = tmpfile();
    if (temp == NULL)
        return 0;

    slash = path == NULL ? NULL : strrchr(path, '/');
    if (slash != NULL) {
        size_t length = (size_t)(slash - path);
        if (length >= sizeof(base))
            length = sizeof(base) - 1;
        memcpy(base, path, length);
        base[length] = '\0';
    } else {
        strcpy(base, ".");
    }

    if (!expand_includes(input, temp, base, 0)) {
        fclose(temp);
        return 0;
    }
    rewind(temp);
    *prepared = temp;
    return 1;
}

static void
usage(const char *program)
{
    fprintf(stderr, "usage: %s [--eval | --ast | --asm] [file]\n", program);
}

static int
is_output_expression(const struct JCMAst *ast)
{
    return ast != NULL && ast->kind == JCM_AST_LIST &&
           ast->value.list.count > 0 &&
           ast->value.list.items[0]->kind == JCM_AST_CORE &&
           ast->value.list.items[0]->value.op == JCM_OUTPUT;
}

/* Check errors that used to become the unhelpful "evaluation failed". */
static int
validate_ast(const struct JCMAst *ast)
{
    int i;

    if (ast == NULL)
        return 1;

    if (ast->kind != JCM_AST_LIST && ast->kind != JCM_AST_PROGRAM)
        return 1;

    if (ast->kind == JCM_AST_LIST) {
        if (ast->value.list.count == 0) {
            fprintf(stderr, "jcm: line %d, column %d: empty expression\n",
                    ast->line, ast->column);
            return 0;
        }

        if (ast->value.list.items[0]->kind == JCM_AST_CORE) {
            const struct JCMCoreForm *form;
            int arguments;

            form = jcm_core_find_op(ast->value.list.items[0]->value.op);
            arguments = ast->value.list.count - 1;
            if (form != NULL &&
                (arguments < form->min_args ||
                 (form->max_args >= 0 && arguments > form->max_args))) {
                fprintf(stderr,
                        "jcm: line %d, column %d: '%s' has %d argument%s; "
                        "correct definition is %s\n",
                        ast->line, ast->column, form->symbol, arguments,
                        arguments == 1 ? "" : "s", form->syntax);
                return 0;
            }
        }
    }

    for (i = 0; i < ast->value.list.count; i++) {
        if (!validate_ast(ast->value.list.items[i]))
            return 0;
    }
    return 1;
}

int
main(int argc, char **argv)
{
    const char *input_path;
    const char *mode;
    FILE *input;
    FILE *prepared;
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
        if (strcmp(argv[i], "--eval") == 0) { mode = "--eval"; continue; }
        if (strcmp(argv[i], "--ast") == 0) { mode = "--ast"; continue; }
        if (strcmp(argv[i], "--asm") == 0) { mode = "--asm"; continue; }
        if (argv[i][0] == '-' || strcmp(input_path, "-") != 0) {
            usage(argv[0]);
            return 2;
        }
        input_path = argv[i];
    }

    input = open_input(input_path);
    if (input == NULL)
        return 1;
    prepared = NULL;
    ok = prepare_source(input, input_path, &prepared);
    if (input != stdin)
        fclose(input);
    if (!ok) {
        fprintf(stderr, "jcm: cannot prepare source\n");
        return 1;
    }

    tokens = NULL;
    token_count = 0;
    ok = jcm_lex(prepared, &tokens, &token_count);
    fclose(prepared);
    if (!ok) {
        fprintf(stderr, "jcm: lexical error (check the reported line and column)\n");
        return 1;
    }
    ast = NULL;
    ok = jcm_ast_parse(tokens, token_count, &ast);
    if (!ok) {
        fprintf(stderr, "jcm: syntax error (check parentheses and the reported line)\n");
        jcm_lex_free(tokens, token_count);
        return 1;
    }
    if (!validate_ast(ast)) {
        jcm_ast_free(ast);
        jcm_lex_free(tokens, token_count);
        return 1;
    }
    if (strcmp(mode, "--ast") == 0) {
        jcm_ast_print(ast, 0);
        jcm_ast_free(ast); jcm_lex_free(tokens, token_count); return 0;
    }
    if (strcmp(mode, "--asm") == 0) {
        ok = jcm_codegen(stdout, ast);
        jcm_ast_free(ast); jcm_lex_free(tokens, token_count); return ok ? 0 : 1;
    }
    if (!jcm_runtime_init(&runtime)) {
        fprintf(stderr, "jcm: cannot initialize runtime\n");
        jcm_ast_free(ast); jcm_lex_free(tokens, token_count); return 1;
    }
    ok = jcm_eval_program(&runtime, ast, &result);
    if (ok && (ast->value.list.count == 0 ||
        !is_output_expression(ast->value.list.items[ast->value.list.count - 1])))
        jcm_value_print(result);
    jcm_runtime_free(&runtime);
    jcm_ast_free(ast); jcm_lex_free(tokens, token_count);
    return ok ? 0 : 1;
}
