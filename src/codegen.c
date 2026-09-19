#include <stdio.h>
#include <stdlib.h>

#include "ast.h"

/*
 * Native x86_64 code generation.
 *
 * This backend currently emits a valid assembly program for the
 * constant-expression subset.  Runtime-dependent forms are rejected
 * rather than silently producing incorrect assembly.
 *
 * This is deliberately small until the runtime ABI is fixed.
 */

static void
codegen_error(const char *message)
{
    fprintf(stderr, "jcm: codegen: %s\n", message);
}

static int
emit_number(FILE *out, long long number)
{
    fprintf(out, "    mov $%lld, %%rax\n", number);
    return 1;
}

static int
emit_expression(FILE *out, const struct JCMAst *ast)
{
    struct JCMAst *left;
    struct JCMAst *right;
    enum JCMOp op;

    if (ast == NULL)
        return 0;

    switch (ast->kind) {

    case JCM_AST_NUMBER:
        return emit_number(out, ast->value.number);

    case JCM_AST_LIST:
        break;

    default:
        codegen_error("expression is not currently representable");
        return 0;
    }

    if (ast->value.list.count == 0)
        return 0;

    if (ast->value.list.items[0]->kind != JCM_AST_CORE) {
        codegen_error("function calls require the runtime backend");
        return 0;
    }

    op = ast->value.list.items[0]->value.op;

    if (ast->value.list.count != 3) {
        codegen_error("native arithmetic currently requires two operands");
        return 0;
    }

    left = ast->value.list.items[1];
    right = ast->value.list.items[2];

    switch (op) {

    case JCM_ADD:
        if (!emit_expression(out, left))
            return 0;

        fprintf(out, "    push %%rax\n");

        if (!emit_expression(out, right))
            return 0;

        fprintf(out, "    mov %%rax, %%rbx\n");
        fprintf(out, "    pop %%rax\n");
        fprintf(out, "    add %%rbx, %%rax\n");

        return 1;

    case JCM_SUB:
        if (!emit_expression(out, left))
            return 0;

        fprintf(out, "    push %%rax\n");

        if (!emit_expression(out, right))
            return 0;

        fprintf(out, "    mov %%rax, %%rbx\n");
        fprintf(out, "    pop %%rax\n");
        fprintf(out, "    sub %%rbx, %%rax\n");

        return 1;

    case JCM_MUL:
        if (!emit_expression(out, left))
            return 0;

        fprintf(out, "    push %%rax\n");

        if (!emit_expression(out, right))
            return 0;

        fprintf(out, "    mov %%rax, %%rbx\n");
        fprintf(out, "    pop %%rax\n");
        fprintf(out, "    imul %%rbx, %%rax\n");

        return 1;

    case JCM_DIV:
        if (!emit_expression(out, left))
            return 0;

        fprintf(out, "    push %%rax\n");

        if (!emit_expression(out, right))
            return 0;

        fprintf(out, "    mov %%rax, %%rbx\n");
        fprintf(out, "    pop %%rax\n");
        fprintf(out, "    cqo\n");
        fprintf(out, "    idiv %%rbx\n");

        return 1;

    case JCM_MOD:
        if (!emit_expression(out, left))
            return 0;

        fprintf(out, "    push %%rax\n");

        if (!emit_expression(out, right))
            return 0;

        fprintf(out, "    mov %%rax, %%rbx\n");
        fprintf(out, "    pop %%rax\n");
        fprintf(out, "    cqo\n");
        fprintf(out, "    idiv %%rbx\n");
        fprintf(out, "    mov %%rdx, %%rax\n");

        return 1;

    default:
        codegen_error("operation requires the JCM runtime");
        return 0;
    }
}

/*
 * Emit one complete x86_64 System V assembly program.
 *
 * Return:
 *     1  success
 *     0  failure
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
    fprintf(out, "    push %%rbp\n");
    fprintf(out, "    mov %%rsp, %%rbp\n");

    if (program->value.list.count == 0) {
        fprintf(out, "    xor %%eax, %%eax\n");
    } else {
        /*
         * Evaluate expressions in order.  The value of the final
         * expression becomes the process exit status.
         */
        for (i = 0; i < program->value.list.count; i++) {
            if (!emit_expression(
                    out,
                    program->value.list.items[i])) {
                fprintf(out, "    mov $1, %%eax\n");
                fprintf(out, "    leave\n");
                fprintf(out, "    ret\n");
                return 0;
            }
        }
    }

    fprintf(out, "    leave\n");
    fprintf(out, "    ret\n");
    fprintf(out, ".size main, .-main\n");

    return 1;
}

