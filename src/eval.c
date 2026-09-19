#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "eval.h"

static char *
jcm_strdup(const char *s)
{
    size_t n;
    char *copy;

    if (s == NULL)
        return NULL;

    n = strlen(s);
    copy = (char *)malloc(n + 1);
    if (copy == NULL)
        return NULL;

    memcpy(copy, s, n + 1);
    return copy;
}

struct JCMEnv *
jcm_env_new(struct JCMEnv *parent)
{
    struct JCMEnv *env;

    env = (struct JCMEnv *)malloc(sizeof(struct JCMEnv));
    if (env == NULL)
        return NULL;

    env->bindings = NULL;
    env->parent = parent;
    return env;
}

void
jcm_env_free(struct JCMEnv *env)
{
    struct JCMBinding *binding;
    struct JCMBinding *next;

    if (env == NULL)
        return;

    binding = env->bindings;
    while (binding != NULL) {
        next = binding->next;
        free(binding->name);
        free(binding);
        binding = next;
    }

    free(env);
}

int
jcm_env_define(struct JCMEnv *env, const char *name, struct JCMValue value)
{
    struct JCMBinding *binding;

    if (env == NULL || name == NULL)
        return 0;

    binding = (struct JCMBinding *)malloc(sizeof(struct JCMBinding));
    if (binding == NULL)
        return 0;

    binding->name = jcm_strdup(name);
    if (binding->name == NULL) {
        free(binding);
        return 0;
    }

    binding->value = value;
    binding->next = env->bindings;
    env->bindings = binding;
    return 1;
}

int
jcm_env_set(struct JCMEnv *env, const char *name, struct JCMValue value)
{
    struct JCMEnv *current;
    struct JCMBinding *binding;

    if (env == NULL || name == NULL)
        return 0;

    current = env;
    while (current != NULL) {
        binding = current->bindings;
        while (binding != NULL) {
            if (strcmp(binding->name, name) == 0) {
                binding->value = value;
                return 1;
            }
            binding = binding->next;
        }
        current = current->parent;
    }

    return 0;
}

int
jcm_env_get(struct JCMEnv *env, const char *name, struct JCMValue *value)
{
    struct JCMEnv *current;
    struct JCMBinding *binding;

    if (env == NULL || name == NULL || value == NULL)
        return 0;

    current = env;
    while (current != NULL) {
        binding = current->bindings;
        while (binding != NULL) {
            if (strcmp(binding->name, name) == 0) {
                *value = binding->value;
                return 1;
            }
            binding = binding->next;
        }
        current = current->parent;
    }

    return 0;
}

struct JCMValue
jcm_value_number(long number)
{
    struct JCMValue value;
    value.kind = JCM_VALUE_NUMBER;
    value.value.number = number;
    return value;
}

struct JCMValue
jcm_value_string(const char *text)
{
    struct JCMValue value;
    value.kind = JCM_VALUE_STRING;
    value.value.string = jcm_strdup(text);
    return value;
}

struct JCMValue
jcm_value_function(struct JCMFunction *function)
{
    struct JCMValue value;
    value.kind = JCM_VALUE_FUNCTION;
    value.value.function = function;
    return value;
}

int
jcm_runtime_init(struct JCMRuntime *runtime)
{
    if (runtime == NULL)
        return 0;

    runtime->global = jcm_env_new(NULL);
    if (runtime->global == NULL)
        return 0;

    runtime->memory.cells = NULL;
    runtime->memory.next_address = 1;
    return 1;
}

void
jcm_runtime_free(struct JCMRuntime *runtime)
{
    struct JCMMemoryCell *cell;
    struct JCMMemoryCell *next;

    if (runtime == NULL)
        return;

    if (runtime->global != NULL)
        jcm_env_free(runtime->global);

    runtime->global = NULL;

    cell = runtime->memory.cells;
    while (cell != NULL) {
        next = cell->next;
        free(cell);
        cell = next;
    }

    runtime->memory.cells = NULL;
    runtime->memory.next_address = 0;
}

int
jcm_memory_load(struct JCMMemory *memory, long address, struct JCMValue *value)
{
    struct JCMMemoryCell *cell;

    if (memory == NULL || value == NULL)
        return 0;

    cell = memory->cells;
    while (cell != NULL) {
        if (cell->address == address) {
            *value = cell->value;
            return 1;
        }
        cell = cell->next;
    }

    return 0;
}

int
jcm_memory_store(struct JCMMemory *memory, long address, struct JCMValue value)
{
    struct JCMMemoryCell *cell;

    if (memory == NULL)
        return 0;

    cell = memory->cells;
    while (cell != NULL) {
        if (cell->address == address) {
            cell->value = value;
            return 1;
        }
        cell = cell->next;
    }

    cell = (struct JCMMemoryCell *)malloc(sizeof(struct JCMMemoryCell));
    if (cell == NULL)
        return 0;

    cell->address = address;
    cell->value = value;
    cell->next = memory->cells;
    memory->cells = cell;
    return 1;
}

static int
eval_number(struct JCMRuntime *runtime, struct JCMEnv *env, const struct JCMAst *ast, struct JCMValue *result)
{
    (void)runtime;
    (void)env;
    result->kind = JCM_VALUE_NUMBER;
    result->value.number = ast->value.number;
    return 1;
}

static int
eval_string(struct JCMRuntime *runtime, struct JCMEnv *env, const struct JCMAst *ast, struct JCMValue *result)
{
    (void)runtime;
    (void)env;
    *result = jcm_value_string(ast->value.string);
    return result->value.string != NULL;
}

static int
eval_symbol(struct JCMRuntime *runtime, struct JCMEnv *env, const struct JCMAst *ast, struct JCMValue *result)
{
    (void)runtime;
    return jcm_env_get(env, ast->value.symbol, result);
}

static int
eval_args(struct JCMRuntime *runtime, struct JCMEnv *env, const struct JCMAst *ast, struct JCMValue *args)
{
    int i;

    for (i = 1; i < ast->value.list.count; i++) {
        if (!jcm_eval(runtime, env, ast->value.list.items[i], &args[i - 1]))
            return 0;
    }
    return 1;
}

static int
eval_binary_number(struct JCMRuntime *runtime, struct JCMEnv *env, const struct JCMAst *ast, enum JCMOp op, struct JCMValue *result)
{
    struct JCMValue args[2];
    long a;
    long b;

    if (ast->value.list.count != 3)
        return 0;
    if (!eval_args(runtime, env, ast, args))
        return 0;
    if (args[0].kind != JCM_VALUE_NUMBER || args[1].kind != JCM_VALUE_NUMBER)
        return 0;

    a = args[0].value.number;
    b = args[1].value.number;

    switch (op) {
    case JCM_ADD:
        *result = jcm_value_number(a + b);
        return 1;
    case JCM_SUB:
        *result = jcm_value_number(a - b);
        return 1;
    case JCM_MUL:
        *result = jcm_value_number(a * b);
        return 1;
    case JCM_DIV:
        if (b == 0)
            return 0;
        *result = jcm_value_number(a / b);
        return 1;
    case JCM_MOD:
        if (b == 0)
            return 0;
        *result = jcm_value_number(a % b);
        return 1;
    default:
        return 0;
    }
}

static int
eval_comparison(struct JCMRuntime *runtime, struct JCMEnv *env, const struct JCMAst *ast, enum JCMOp op, struct JCMValue *result)
{
    struct JCMValue args[2];
    long a;
    long b;
    long answer;

    if (ast->value.list.count != 3)
        return 0;
    if (!eval_args(runtime, env, ast, args))
        return 0;
    if (args[0].kind != JCM_VALUE_NUMBER || args[1].kind != JCM_VALUE_NUMBER)
        return 0;

    a = args[0].value.number;
    b = args[1].value.number;
    answer = 0;

    switch (op) {
    case JCM_EQ:
        answer = (a == b);
        break;
    case JCM_LT:
        answer = (a < b);
        break;
    case JCM_LE:
        answer = (a <= b);
        break;
    case JCM_GT:
        answer = (a > b);
        break;
    case JCM_GE:
        answer = (a >= b);
        break;
    default:
        return 0;
    }

    *result = jcm_value_number(answer);
    return 1;
}

static int
eval_logic(struct JCMRuntime *runtime, struct JCMEnv *env, const struct JCMAst *ast, enum JCMOp op, struct JCMValue *result)
{
    struct JCMValue a;
    struct JCMValue b;
    int truth_a;
    int truth_b;

    if (op == JCM_NOT) {
        if (ast->value.list.count != 2)
            return 0;
        if (!jcm_eval(runtime, env, ast->value.list.items[1], &a))
            return 0;
        if (a.kind != JCM_VALUE_NUMBER)
            return 0;

        *result = jcm_value_number(a.value.number == 0 ? 1 : 0);
        return 1;
    }

    if (ast->value.list.count != 3)
        return 0;
    if (!jcm_eval(runtime, env, ast->value.list.items[1], &a))
        return 0;
    if (a.kind != JCM_VALUE_NUMBER)
        return 0;

    truth_a = a.value.number != 0;
    if (op == JCM_AND && !truth_a) {
        *result = jcm_value_number(0);
        return 1;
    }
    if (op == JCM_OR && truth_a) {
        *result = jcm_value_number(1);
        return 1;
    }

    if (!jcm_eval(runtime, env, ast->value.list.items[2], &b))
        return 0;
    if (b.kind != JCM_VALUE_NUMBER)
        return 0;

    truth_b = b.value.number != 0;
    if (op == JCM_AND)
        *result = jcm_value_number(truth_a && truth_b);
    else
        *result = jcm_value_number(truth_a || truth_b);

    return 1;
}

static int
eval_if(struct JCMRuntime *runtime, struct JCMEnv *env, const struct JCMAst *ast, struct JCMValue *result)
{
    struct JCMValue condition;

    if (ast->value.list.count != 4)
        return 0;
    if (!jcm_eval(runtime, env, ast->value.list.items[1], &condition))
        return 0;
    if (condition.kind != JCM_VALUE_NUMBER)
        return 0;

    if (condition.value.number != 0)
        return jcm_eval(runtime, env, ast->value.list.items[2], result);

    return jcm_eval(runtime, env, ast->value.list.items[3], result);
}

static int
eval_lambda(struct JCMRuntime *runtime, struct JCMEnv *env, const struct JCMAst *ast, struct JCMValue *result)
{
    struct JCMFunction *function;

    (void)runtime;

    if (ast->value.list.count != 3)
        return 0;
    if (ast->value.list.items[1]->kind != JCM_AST_LIST)
        return 0;
    if (ast->value.list.items[1]->value.list.count != 1)
        return 0;
    if (ast->value.list.items[1]->value.list.items[0]->kind != JCM_AST_SYMBOL)
        return 0;

    function = (struct JCMFunction *)malloc(sizeof(struct JCMFunction));
    if (function == NULL)
        return 0;

    function->lambda = ast;
    function->env = env;
    *result = jcm_value_function(function);
    return 1;
}

static int
eval_bind(struct JCMRuntime *runtime, struct JCMEnv *env, const struct JCMAst *ast, struct JCMValue *result)
{
    const struct JCMAst *name;
    const struct JCMAst *expression;
    struct JCMValue value;

    if (ast->value.list.count != 3)
        return 0;

    if (ast->value.list.items[0]->kind == JCM_AST_CORE && ast->value.list.items[0]->value.op == JCM_BIND) {
        name = ast->value.list.items[1];
        expression = ast->value.list.items[2];
    } else if (ast->value.list.items[1]->kind == JCM_AST_CORE &&
               ast->value.list.items[1]->value.op == JCM_BIND) {
        name = ast->value.list.items[0];
        expression = ast->value.list.items[2];
    } else {
        return 0;
    }

    if (name->kind != JCM_AST_SYMBOL)
        return 0;
    if (!jcm_eval(runtime, env, expression, &value))
        return 0;
    if (!jcm_env_define(env, name->value.symbol, value))
        return 0;

    *result = value;
    return 1;
}

static int
eval_call(struct JCMRuntime *runtime, struct JCMEnv *env, const struct JCMAst *ast, struct JCMValue *result)
{
    struct JCMValue function_value;
    struct JCMFunction *function;
    const struct JCMAst *lambda;
    const struct JCMAst *parameters;
    const struct JCMAst *parameter;
    struct JCMEnv *call_env;
    struct JCMValue argument;

    if (ast->value.list.count < 1)
        return 0;
    if (!jcm_eval(runtime, env, ast->value.list.items[0], &function_value))
        return 0;
    if (function_value.kind != JCM_VALUE_FUNCTION)
        return 0;

    function = function_value.value.function;
    if (function == NULL || function->lambda == NULL)
        return 0;

    lambda = function->lambda;
    if (lambda->value.list.count != 3)
        return 0;

    parameters = lambda->value.list.items[1];
    if (parameters->kind != JCM_AST_LIST || parameters->value.list.count != 1)
        return 0;
    if (ast->value.list.count != 2)
        return 0;

    parameter = parameters->value.list.items[0];
    if (parameter->kind != JCM_AST_SYMBOL)
        return 0;
    if (!jcm_eval(runtime, env, ast->value.list.items[1], &argument))
        return 0;

    call_env = jcm_env_new(function->env);
    if (call_env == NULL)
        return 0;

    if (!jcm_env_define(call_env, parameter->value.symbol, argument)) {
        jcm_env_free(call_env);
        return 0;
    }

    return jcm_eval(runtime, call_env, lambda->value.list.items[2], result);
}

static int
eval_load(struct JCMRuntime *runtime, struct JCMEnv *env, const struct JCMAst *ast, struct JCMValue *result)
{
    struct JCMValue address;

    if (ast->value.list.count != 2)
        return 0;
    if (!jcm_eval(runtime, env, ast->value.list.items[1], &address))
        return 0;
    if (address.kind != JCM_VALUE_NUMBER)
        return 0;

    return jcm_memory_load(&runtime->memory, address.value.number, result);
}

static int
eval_store(struct JCMRuntime *runtime, struct JCMEnv *env, const struct JCMAst *ast, struct JCMValue *result)
{
    struct JCMValue address;
    struct JCMValue value;

    if (ast->value.list.count != 3)
        return 0;
    if (!jcm_eval(runtime, env, ast->value.list.items[1], &address))
        return 0;
    if (!jcm_eval(runtime, env, ast->value.list.items[2], &value))
        return 0;
    if (address.kind != JCM_VALUE_NUMBER)
        return 0;
    if (!jcm_memory_store(&runtime->memory, address.value.number, value))
        return 0;

    *result = value;
    return 1;
}

static int
eval_input(struct JCMRuntime *runtime, struct JCMEnv *env, const struct JCMAst *ast, struct JCMValue *result)
{
    long value;

    (void)runtime;
    (void)env;

    if (ast->value.list.count != 1)
        return 0;
    if (scanf("%ld", &value) != 1)
        return 0;

    *result = jcm_value_number(value);
    return 1;
}

static int
eval_output(struct JCMRuntime *runtime, struct JCMEnv *env, const struct JCMAst *ast, struct JCMValue *result)
{
    struct JCMValue value;

    if (ast->value.list.count != 2)
        return 0;
    if (!jcm_eval(runtime, env, ast->value.list.items[1], &value))
        return 0;

    if (value.kind == JCM_VALUE_NUMBER) {
        printf("%ld", value.value.number);
    } else if (value.kind == JCM_VALUE_STRING) {
        fputs(value.value.string, stdout);
    } else {
        return 0;
    }

    *result = value;
    return 1;
}

int
jcm_eval(struct JCMRuntime *runtime, struct JCMEnv *env, const struct JCMAst *ast, struct JCMValue *result)
{
    enum JCMOp op;

    if (runtime == NULL || env == NULL || ast == NULL || result == NULL)
        return 0;

    switch (ast->kind) {
    case JCM_AST_NUMBER:
        return eval_number(runtime, env, ast, result);

    case JCM_AST_STRING:
        return eval_string(runtime, env, ast, result);

    case JCM_AST_SYMBOL:
        return eval_symbol(runtime, env, ast, result);

    case JCM_AST_CORE:
        return 0;

    case JCM_AST_PROGRAM:
        return jcm_eval_program(runtime, ast, result);

    case JCM_AST_LIST:
        break;
    }

    if (ast->value.list.count == 0)
        return 0;

    if (ast->value.list.count == 3 &&
        ast->value.list.items[1]->kind == JCM_AST_CORE &&
        ast->value.list.items[1]->value.op == JCM_BIND) {
        return eval_bind(runtime, env, ast, result);
    }

    if (ast->value.list.items[0]->kind == JCM_AST_CORE) {
        op = ast->value.list.items[0]->value.op;
        switch (op) {
        case JCM_AND:
        case JCM_OR:
        case JCM_NOT:
            return eval_logic(runtime, env, ast, op, result);

        case JCM_EQ:
        case JCM_LT:
        case JCM_LE:
        case JCM_GT:
        case JCM_GE:
            return eval_comparison(runtime, env, ast, op, result);

        case JCM_ADD:
        case JCM_SUB:
        case JCM_MUL:
        case JCM_DIV:
        case JCM_MOD:
            return eval_binary_number(runtime, env, ast, op, result);

        case JCM_IF:
            return eval_if(runtime, env, ast, result);

        case JCM_LAMBDA:
            return eval_lambda(runtime, env, ast, result);

        case JCM_BIND:
            return eval_bind(runtime, env, ast, result);

        case JCM_LOAD:
            return eval_load(runtime, env, ast, result);

        case JCM_STORE:
            return eval_store(runtime, env, ast, result);

        case JCM_INPUT:
            return eval_input(runtime, env, ast, result);

        case JCM_OUTPUT:
            return eval_output(runtime, env, ast, result);
        }
    }

    return eval_call(runtime, env, ast, result);
}

int
jcm_eval_program(struct JCMRuntime *runtime, const struct JCMAst *program, struct JCMValue *result)
{
    struct JCMValue value;
    int i;

    if (runtime == NULL || program == NULL || result == NULL)
        return 0;
    if (program->kind != JCM_AST_PROGRAM)
        return 0;

    value = jcm_value_number(0);
    for (i = 0; i < program->value.list.count; i++) {
        if (!jcm_eval(runtime, runtime->global, program->value.list.items[i], &value))
            return 0;
    }

    *result = value;
    return 1;
}

void
jcm_value_print(struct JCMValue value)
{
    switch (value.kind) {
    case JCM_VALUE_NUMBER:
        printf("%ld\n", value.value.number);
        break;

    case JCM_VALUE_BOOLEAN:
        printf("%ld\n", value.value.number ? 1L : 0L);
        break;

    case JCM_VALUE_STRING:
        printf("%s\n", value.value.string);
        break;

    case JCM_VALUE_FUNCTION:
        printf("<function>\n");
        break;
    }
}
