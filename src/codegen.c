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
 * The idea is simple:
 *   - every expression leaves its result in %rax
 *   - binary operations evaluate the left operand first,
 *     save it on the stack, evaluate the right operand, then combine
 *     the two values in %rax and %rbx
 *
 * Anything that needs runtime state (functions, variables, memory,
 * control flow, or I/O) is rejected instead of generating misleading
 * assembly.
 */

static void
codegen_error(const char *message)
{
    fprintf(stderr, "jcm: codegen: %s\n", message);
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

    /*
     * Every binary operation follows this easy-to-read pattern:
     *
     *   evaluate left       -> %rax
     *   save left           -> push %rax
     *   evaluate right      -> %rax
     *   move right to %rbx
     *   restore left        -> pop %rax
     *   combine %rax/%rbx
     *
     * At the end, %rax contains the operation's result.
     */
    if (!emit_expression(out, left))
        return 0;
    fprintf(out, "    push %%rax\n");

    if (!emit_expression(out, right))
        return 0;
    fprintf(out, "    mov %%rax, %%rbx\n");
    fprintf(out, "    pop %%rax\n");

    switch (op) {
    case JCM_ADD:
        fprintf(out, "    add %%rbx, %%rax\n");
        return 1;

    case JCM_SUB:
        fprintf(out, "    sub %%rbx, %%rax\n");
        return 1;

    case JCM_MUL:
        fprintf(out, "    imul %%rbx, %%rax\n");
        return 1;

    case JCM_DIV:
        /* idiv divides the signed 128-bit value in %rdx:%rax by %rbx. */
        fprintf(out, "    cqo\n");
        fprintf(out, "    idiv %%rbx\n");
        return 1;

    case JCM_MOD:
        /* idiv leaves the remainder in %rdx, so move it into %rax. */
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

    /* Standard function prologue: create a stack frame. */
    fprintf(out, "    push %%rbp\n");
    fprintf(out, "    mov %%rsp, %%rbp\n");

    if (program->value.list.count == 0) {
        /* An empty program returns zero. */
        fprintf(out, "    xor %%eax, %%eax\n");
    } else {
        for (i = 0; i < program->value.list.count; i++) {
            if (!emit_expression(out, program->value.list.items[i])) {
                /* Return a nonzero status if code generation fails. */
                fprintf(out, "    mov $1, %%eax\n");
                fprintf(out, "    leave\n");
                fprintf(out, "    ret\n");
                return 0;
            }
        }
    }

    /* Standard function epilogue: restore the stack frame and return. */
    fprintf(out, "    leave\n");
    fprintf(out, "    ret\n");
    fprintf(out, ".size main, .-main\n");

    return 1;
}
