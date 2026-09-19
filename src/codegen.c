#include <stdio.h>

#include "ast.h"

static void
codegen_error(const char *message)
{
    fprintf(stderr, "jcm: codegen: %s\n", message);
}

static const char *
runtime_symbol(enum JCMOp op)
{
    switch (op) {
    case JCM_AND:    return "jcm_runtime_and";
    case JCM_OR:     return "jcm_runtime_or";
    case JCM_NOT:    return "jcm_runtime_not";
    case JCM_EQ:     return "jcm_runtime_eq";
    case JCM_LT:     return "jcm_runtime_lt";
    case JCM_LE:     return "jcm_runtime_le";
    case JCM_GT:     return "jcm_runtime_gt";
    case JCM_GE:     return "jcm_runtime_ge";
    case JCM_IF:     return "jcm_runtime_if";
    case JCM_LAMBDA: return "jcm_runtime_lambda";
    case JCM_BIND:   return "jcm_runtime_bind";
    case JCM_LOAD:   return "jcm_runtime_load";
    case JCM_STORE:  return "jcm_runtime_store";
    case JCM_INPUT:  return "jcm_runtime_input";
    case JCM_OUTPUT: return "jcm_runtime_output";
    default:         return "jcm_runtime_generic";
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

static int emit_expression(FILE *out, const struct JCMAst *ast);

static int
emit_binary_arithmetic(
    FILE *out,
    const struct JCMAst *left,
    const struct JCMAst *right,
    enum JCMOp op
)
{
    /*
     * Evaluate both operands before using either result:
     *
     *     left  -> push
     *     right -> push
     *     pop right
     *     pop left
     *
     * This works for nested expressions and does not use callee-saved
     * registers such as %rbx.
     */
    if (!emit_expression(out, left))
        return 0;
    fprintf(out, "    push %%rax\n");

    if (!emit_expression(out, right))
        return 0;
    fprintf(out, "    push %%rax\n");

    fprintf(out, "    pop %%rcx\n");
    fprintf(out, "    pop %%rax\n");

    switch (op) {
    case JCM_ADD:
        fprintf(out, "    add %%rcx, %%rax\n");
        return 1;

    case JCM_SUB:
        fprintf(out, "    sub %%rcx, %%rax\n");
        return 1;

    case JCM_MUL:
        fprintf(out, "    imul %%rcx, %%rax\n");
        return 1;

    case JCM_DIV:
        fprintf(out, "    cqo\n");
        fprintf(out, "    idiv %%rcx\n");
        return 1;

    case JCM_MOD:
        fprintf(out, "    cqo\n");
        fprintf(out, "    idiv %%rcx\n");
        fprintf(out, "    mov %%rdx, %%rax\n");
        return 1;

    default:
        return 0;
    }
}

static int
emit_expression(FILE *out, const struct JCMAst *ast)
{
    const struct JCMAst *left;
    const struct JCMAst *right;
    enum JCMOp op;

    if (ast == NULL)
        return 0;

    switch (ast->kind) {
    case JCM_AST_NUMBER:
        return emit_number(out, ast->value.number);

    case JCM_AST_STRING:
        /*
         * Strings are not currently represented in native assembly.
         * Return zero for the constant backend.
         */
        fprintf(out, "    xor %%eax, %%eax\n");
        return 1;

    case JCM_AST_LIST:
        break;

    default:
        codegen_error("expression is not currently representable");
        return 0;
    }

    if (ast->value.list.count == 0) {
        codegen_error("cannot generate an empty expression");
        return 0;
    }

    if (ast->value.list.items[0] == NULL ||
        ast->value.list.items[0]->kind != JCM_AST_CORE) {
        codegen_error("function calls require the runtime backend");
        return 0;
    }

    op = ast->value.list.items[0]->value.op;

    if (op != JCM_ADD &&
        op != JCM_SUB &&
        op != JCM_MUL &&
        op != JCM_DIV &&
        op != JCM_MOD)
        return emit_runtime_expression(out, op);

    if (ast->value.list.count != 3) {
        codegen_error("arithmetic expressions require two operands");
        return 0;
    }

    left = ast->value.list.items[1];
    right = ast->value.list.items[2];

    return emit_binary_arithmetic(out, left, right, op);
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
