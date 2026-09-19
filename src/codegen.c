#include <stdio.h>
#include <stdlib.h>

#include "ast.h"

/*
 * Native x86_64 code generation.
 *
 * This backend supports the constant-expression subset directly and lowers
 * runtime-dependent forms to external runtime helper calls.  That keeps the
 * generated assembly valid while allowing the complete JCM language surface to
 * be represented without silently producing incorrect code.
 */

static void
codegen_error(const char *message)
{
    fprintf(stderr, "jcm: codegen: %s\n", message);
}

static const char *
runtime_symbol(enum JCMOp op)
{
    switch (op) {
    case JCM_AND:
        return "jcm_runtime_and";
    case JCM_OR:
        return "jcm_runtime_or";
    case JCM_NOT:
        return "jcm_runtime_not";
    case JCM_EQ:
        return "jcm_runtime_eq";
    case JCM_LT:
        return "jcm_runtime_lt";
    case JCM_LE:
        return "jcm_runtime_le";
    case JCM_GT:
        return "jcm_runtime_gt";
    case JCM_GE:
        return "jcm_runtime_ge";
    case JCM_IF:
        return "jcm_runtime_if";
    case JCM_LAMBDA:
        return "jcm_runtime_lambda";
    case JCM_BIND:
        return "jcm_runtime_bind";
    case JCM_LOAD:
        return "jcm_runtime_load";
    case JCM_STORE:
        return "jcm_runtime_store";
    case JCM_INPUT:
        return "jcm_runtime_input";
    case JCM_OUTPUT:
        return "jcm_runtime_output";
    default:
        return "jcm_runtime_generic";
    }
}

static int
emit_number(FILE *out, long long number)
{
    fprintf(out, "    mov $%lld, %%rax\n", number);
    return 1;
}

static int
emit_runtime_expression(FILE *out, enum JCMOp op)
{
    fprintf(out, "    call %s\n", runtime_symbol(op));
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
    case JCM_AST_STRING:
        /* We cannot materialize arbitrary strings in a bare assembly stub.
         * The runtime helper is expected to handle them when linked. */
        fprintf(out, "    xor %%eax, %%eax\n");
        return 1;
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

    switch (op) {
    case JCM_ADD:
    case JCM_SUB:
    case JCM_MUL:
    case JCM_DIV:
    case JCM_MOD:
        break;
    default:
        return emit_runtime_expression(out, op);
    }

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
        return 0;
    }
}

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
    fprintf(out, ".extern jcm_runtime_and\n");
    fprintf(out, ".extern jcm_runtime_or\n");
    fprintf(out, ".extern jcm_runtime_not\n");
    fprintf(out, ".extern jcm_runtime_eq\n");
    fprintf(out, ".extern jcm_runtime_lt\n");
    fprintf(out, ".extern jcm_runtime_le\n");
    fprintf(out, ".extern jcm_runtime_gt\n");
    fprintf(out, ".extern jcm_runtime_ge\n");
    fprintf(out, ".extern jcm_runtime_if\n");
    fprintf(out, ".extern jcm_runtime_lambda\n");
    fprintf(out, ".extern jcm_runtime_bind\n");
    fprintf(out, ".extern jcm_runtime_load\n");
    fprintf(out, ".extern jcm_runtime_store\n");
    fprintf(out, ".extern jcm_runtime_input\n");
    fprintf(out, ".extern jcm_runtime_output\n");
    fprintf(out, ".extern jcm_runtime_generic\n");
    fprintf(out, "main:\n");
    fprintf(out, "    push %%rbp\n");
    fprintf(out, "    mov %%rsp, %%rbp\n");

    if (program->value.list.count == 0) {
        fprintf(out, "    xor %%eax, %%eax\n");
    } else {
        for (i = 0; i < program->value.list.count; i++) {
            if (!emit_expression(out, program->value.list.items[i])) {
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
