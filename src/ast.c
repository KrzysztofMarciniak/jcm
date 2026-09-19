#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ast.h"

static char *
jcm_ast_strdup(const char *s)
{
    size_t length;
    char *copy;

    if (s == NULL)
        return NULL;

    length = strlen(s);
    copy = (char *)malloc(length + 1);

    if (copy == NULL)
        return NULL;

    memcpy(copy, s, length + 1);
    return copy;
}

static struct JCMAst *
ast_new(enum JCMAstKind kind, int line, int column)
{
    struct JCMAst *node;

    node = (struct JCMAst *)malloc(sizeof(struct JCMAst));
    if (node == NULL)
        return NULL;

    node->kind = kind;
    node->line = line;
    node->column = column;
    return node;
}

static struct JCMAst *
ast_number(long long number, int line, int column)
{
    struct JCMAst *node;

    node = ast_new(JCM_AST_NUMBER, line, column);
    if (node == NULL)
        return NULL;

    node->value.number = number;
    return node;
}

static struct JCMAst *
ast_string(const char *text, int line, int column)
{
    struct JCMAst *node;

    node = ast_new(JCM_AST_STRING, line, column);
    if (node == NULL)
        return NULL;

    node->value.string = jcm_ast_strdup(text);
    if (node->value.string == NULL) {
        free(node);
        return NULL;
    }
    return node;
}

static struct JCMAst *
ast_symbol(const char *symbol, int line, int column)
{
    struct JCMAst *node;

    node = ast_new(JCM_AST_SYMBOL, line, column);
    if (node == NULL)
        return NULL;

    node->value.symbol = jcm_ast_strdup(symbol);
    if (node->value.symbol == NULL) {
        free(node);
        return NULL;
    }
    return node;
}

static struct JCMAst *
ast_core(enum JCMOp op, int line, int column)
{
    struct JCMAst *node;

    node = ast_new(JCM_AST_CORE, line, column);
    if (node == NULL)
        return NULL;

    node->value.op = op;
    return node;
}

static struct JCMAst *
ast_container(enum JCMAstKind kind, int line, int column)
{
    struct JCMAst *node;

    node = ast_new(kind, line, column);
    if (node == NULL)
        return NULL;

    node->value.list.items = NULL;
    node->value.list.count = 0;
    return node;
}

static int
ast_append(struct JCMAst *list, struct JCMAst *child)
{
    struct JCMAst **new_items;
    int new_count;

    new_count = list->value.list.count + 1;
    new_items = (struct JCMAst **)realloc(
        list->value.list.items,
        sizeof(struct JCMAst *) * (size_t)new_count
    );
    if (new_items == NULL)
        return 0;

    list->value.list.items = new_items;
    list->value.list.items[list->value.list.count] = child;
    list->value.list.count = new_count;
    return 1;
}

struct JCMParser {
    const struct JCMToken *tokens;
    int count;
    int position;
};

static struct JCMAst *parse_expression(struct JCMParser *parser);
static struct JCMAst *parse_list(struct JCMParser *parser);

static struct JCMAst *
parse_symbol(const struct JCMToken *token)
{
    const struct JCMCoreForm *core;

    core = jcm_core_find(token->text);
    if (core != NULL)
        return ast_core(core->op, token->line, token->column);

    return ast_symbol(token->text, token->line, token->column);
}

static struct JCMAst *
parse_expression(struct JCMParser *parser)
{
    const struct JCMToken *token;

    if (parser->position >= parser->count)
        return NULL;

    token = &parser->tokens[parser->position];

    switch (token->kind) {
    case JCM_TOKEN_NUMBER:
        parser->position++;
        return ast_number(token->number, token->line, token->column);

    case JCM_TOKEN_STRING:
        parser->position++;
        return ast_string(token->text, token->line, token->column);

    case JCM_TOKEN_SYMBOL:
        parser->position++;
        return parse_symbol(token);

    case JCM_TOKEN_LPAREN:
        return parse_list(parser);

    case JCM_TOKEN_RPAREN:
        return NULL;

    case JCM_TOKEN_EOF:
        return NULL;
    }

    return NULL;
}

static struct JCMAst *
parse_list(struct JCMParser *parser)
{
    const struct JCMToken *open;
    struct JCMAst *list;

    open = &parser->tokens[parser->position];
    if (open->kind != JCM_TOKEN_LPAREN)
        return NULL;

    parser->position++;
    list = ast_container(JCM_AST_LIST, open->line, open->column);
    if (list == NULL)
        return NULL;

    while (parser->position < parser->count) {
        const struct JCMToken *token;
        struct JCMAst *child;

        token = &parser->tokens[parser->position];

        if (token->kind == JCM_TOKEN_RPAREN) {
            parser->position++;
            return list;
        }

        if (token->kind == JCM_TOKEN_EOF) {
            jcm_ast_free(list);
            return NULL;
        }

        child = parse_expression(parser);
        if (child == NULL) {
            jcm_ast_free(list);
            return NULL;
        }

        if (!ast_append(list, child)) {
            jcm_ast_free(child);
            jcm_ast_free(list);
            return NULL;
        }
    }

    jcm_ast_free(list);
    return NULL;
}

int
jcm_ast_parse(
    const struct JCMToken *tokens,
    int token_count,
    struct JCMAst **ast
)
{
    struct JCMParser parser;
    struct JCMAst *program;

    if (tokens == NULL || token_count <= 0 || ast == NULL)
        return 0;

    parser.tokens = tokens;
    parser.count = token_count;
    parser.position = 0;

    program = ast_container(JCM_AST_PROGRAM, tokens[0].line, tokens[0].column);
    if (program == NULL)
        return 0;

    while (parser.position < parser.count) {
        const struct JCMToken *token;
        struct JCMAst *expression;

        token = &parser.tokens[parser.position];
        if (token->kind == JCM_TOKEN_EOF)
            break;

        expression = parse_expression(&parser);
        if (expression == NULL) {
            jcm_ast_free(program);
            return 0;
        }

        if (!ast_append(program, expression)) {
            jcm_ast_free(expression);
            jcm_ast_free(program);
            return 0;
        }
    }

    if (parser.position >= parser.count ||
        parser.tokens[parser.position].kind != JCM_TOKEN_EOF) {
        jcm_ast_free(program);
        return 0;
    }

    *ast = program;
    return 1;
}

void
jcm_ast_free(struct JCMAst *ast)
{
    int i;

    if (ast == NULL)
        return;

    switch (ast->kind) {
    case JCM_AST_NUMBER:
        break;

    case JCM_AST_STRING:
        free(ast->value.string);
        break;

    case JCM_AST_SYMBOL:
        free(ast->value.symbol);
        break;

    case JCM_AST_CORE:
        break;

    case JCM_AST_LIST:
    case JCM_AST_PROGRAM:
        for (i = 0; i < ast->value.list.count; i++)
            jcm_ast_free(ast->value.list.items[i]);
        free(ast->value.list.items);
        break;
    }

    free(ast);
}

static void
print_indent(int indent)
{
    int i;

    for (i = 0; i < indent; i++)
        printf("  ");
}

static const char *
ast_core_name(enum JCMOp op)
{
    const struct JCMCoreForm *core;

    core = jcm_core_find_op(op);
    if (core == NULL)
        return "?";

    return core->symbol;
}

void
jcm_ast_print(const struct JCMAst *ast, int indent)
{
    int i;

    if (ast == NULL) {
        print_indent(indent);
        printf("(null)\n");
        return;
    }

    switch (ast->kind) {
    case JCM_AST_NUMBER:
        print_indent(indent);
        printf("NUMBER %lld\n", ast->value.number);
        break;

    case JCM_AST_STRING:
        print_indent(indent);
        printf("STRING %s\n", ast->value.string);
        break;

    case JCM_AST_SYMBOL:
        print_indent(indent);
        printf("SYMBOL %s\n", ast->value.symbol);
        break;

    case JCM_AST_CORE:
        print_indent(indent);
        printf("CORE %s\n", ast_core_name(ast->value.op));
        break;

    case JCM_AST_LIST:
        print_indent(indent);
        printf("LIST\n");
        for (i = 0; i < ast->value.list.count; i++)
            jcm_ast_print(ast->value.list.items[i], indent + 1);
        break;

    case JCM_AST_PROGRAM:
        print_indent(indent);
        printf("PROGRAM\n");
        for (i = 0; i < ast->value.list.count; i++)
            jcm_ast_print(ast->value.list.items[i], indent + 1);
        break;
    }
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
