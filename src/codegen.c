#include <stdio.h>
#include <stdlib.h>

#include "ast.h"

/*
 * Native x86_64 code generation.
 *
 * This backend intentionally supports only a small subset of JCM:
 *   - numeric literals
 *   - arithmetic on constant values
 *
 * The generated assembly is deliberately simple and easy to read:
 *   - each expression is commented with the original JCM source
 *   - every expression leaves its result in %rax
 *   - binary operations evaluate left, save it, evaluate right, restore left,
 *     and then combine the values
 *
 * Anything that needs runtime state (variables, memory, control flow, I/O,
 * functions, or binding) is rejected with a clear message.
 */

static void
codegen_error(const char *message)
{
    fprintf(stderr, "jcm: codegen: %s\n", message);
}

static void
print_jcm_ast(FILE *out, const struct JCMAst *ast)
{
    int i;
    const struct JCMCoreForm *core;

    if (ast == NULL) {
        fprintf(out, "<null>");
        return;
    }

    switch (ast->kind) {
    case JCM_AST_NUMBER:
        fprintf(out, "%lld", ast->value.number);
        break;

    case JCM_AST_SYMBOL:
        fprintf(out, "%s", ast->value.symbol);
        break;

    case JCM_AST_CORE:
        core = jcm_core_find_op(ast->value.op);
        if (core != NULL)
            fprintf(out, "%s", core->symbol);
        else
            fprintf(out, "?");
        break;

    case JCM_AST_LIST:
        fprintf(out, "(");
        for (i = 0; i < ast->value.list.count; i++) {
            if (i > 0)
                fprintf(out, " ");
            print_jcm_ast(out, ast->value.list.items[i]);
        }
        fprintf(out, ")");
        break;

    case JCM_AST_PROGRAM:
        fprintf(out, "(");
        for (i = 0; i < ast->value.list.count; i++) {
            if (i > 0)
                fprintf(out, " ");
            print_jcm_ast(out, ast->value.list.items[i]);
        }
        fprintf(out, ")");
        break;
    }
}

static void
emit_source_comment(FILE *out, const struct JCMAst *ast)
{
    fprintf(out, "# JCM source: ");
    print_jcm_ast(out, ast);
    fprintf(out, "\n");
}

/* Emit a constant as an immediate value in %rax. */
static int
emit_number(FILE *out, long long number)
{
    fprintf(out, "    mov $%lld, %%rax\n", number);
    return 1;
}

/*
 * Emit one expression.
 *
 * Convention: every successful expression leaves its result in %rax.
 */
static int
emit_expression(FILE *out, const struct JCMAst *ast)
{
    const struct JCMAst *left;
    const struct JCMAst *right;
    enum JCMOp op;

    if (ast == NULL)
        return 0;

    emit_source_comment(out, ast);

    switch (ast->kind) {
    case JCM_AST_NUMBER:
        return emit_number(out, ast->value.number);

    case JCM_AST_LIST:
        break;

    default:
        fprintf(out, "# This form needs runtime support.\n");
        codegen_error("expression is not currently representable");
        return 0;
    }

    if (ast->value.list.count == 0)
        return 0;

    if (ast->value.list.items[0]->kind != JCM_AST_CORE) {
        fprintf(out, "# This is a function call, so the runtime backend is needed.\n");
        codegen_error("function calls require the runtime backend");
        return 0;
    }

    op = ast->value.list.items[0]->value.op;
    if (ast->value.list.count != 3) {
        fprintf(out, "# This native backend only handles binary arithmetic.\n");
        codegen_error("native arithmetic currently requires two operands");
        return 0;
    }

    left = ast->value.list.items[1];
    right = ast->value.list.items[2];

    /*
     * Binary arithmetic follows the same easy pattern for every operator:
     *
     *   evaluate left  -> %rax
     *   push %rax
     *   evaluate right -> %rax
     *   copy right to %rbx
     *   pop left into %rax
     *   combine values
     *
     * In other words, this is how JCM arithmetic is translated into assembly.
     */
    fprintf(out, "# Evaluate the left operand and keep it on the stack.\n");
    if (!emit_expression(out, left))
        return 0;
    fprintf(out, "    push %%rax\n");

    fprintf(out, "# Evaluate the right operand.\n");
    if (!emit_expression(out, right))
        return 0;
    fprintf(out, "    mov %%rax, %%rbx\n");
    fprintf(out, "    pop %%rax\n");

    switch (op) {
    case JCM_ADD:
        fprintf(out, "# left + right\n");
        fprintf(out, "    add %%rbx, %%rax\n");
        return 1;

    case JCM_SUB:
        fprintf(out, "# left - right\n");
        fprintf(out, "    sub %%rbx, %%rax\n");
        return 1;

    case JCM_MUL:
        fprintf(out, "# left * right\n");
        fprintf(out, "    imul %%rbx, %%rax\n");
        return 1;

    case JCM_DIV:
        fprintf(out, "# left / right (signed divide)\n");
        fprintf(out, "    cqo\n");
        fprintf(out, "    idiv %%rbx\n");
        return 1;

    case JCM_MOD:
        fprintf(out, "# left % right\n");
        fprintf(out, "    cqo\n");
        fprintf(out, "    idiv %%rbx\n");
        fprintf(out, "    mov %%rdx, %%rax\n");
        return 1;

    default:
        fprintf(out, "# This operation still needs the runtime backend.\n");
        codegen_error("operation requires the JCM runtime");
        return 0;
    }
}

/*
 * Emit a complete program.
 *
 * Top-level expressions are emitted in source order. The final expression's
 * value is returned by main as the process exit status.
 */
int
jcm_codegen(FILE *out, const struct JCMAst *program)
{
    int i;

    if (out == NULL || program == NULL)
        return 0;

    if (program->kind != JCM_AST_PROGRAM) {
        codegen_error("expected program AST");
        return 0;
    }

    fprintf(out, ".text\n");
    fprintf(out, ".globl main\n");
    fprintf(out, ".type main, @function\n");
    fprintf(out, "main:\n");
    fprintf(out, "# Standard prologue: create a stack frame.\n");
    fprintf(out, "    push %%rbp\n");
    fprintf(out, "    mov %%rsp, %%rbp\n");

    if (program->value.list.count == 0) {
        fprintf(out, "# Empty program: return 0.\n");
        fprintf(out, "    xor %%eax, %%eax\n");
    } else {
        for (i = 0; i < program->value.list.count; i++) {
            emit_source_comment(out, program->value.list.items[i]);
            if (!emit_expression(out, program->value.list.items[i])) {
                fprintf(out, "# Code generation failed; return an error status.\n");
                fprintf(out, "    mov $1, %%eax\n");
                fprintf(out, "    leave\n");
                fprintf(out, "    ret\n");
                return 0;
            }
        }
    }

    fprintf(out, "# Standard epilogue: restore the frame and return.\n");
    fprintf(out, "    leave\n");
    fprintf(out, "    ret\n");
    fprintf(out, ".size main, .-main\n");

    return 1;
}
