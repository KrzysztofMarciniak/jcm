#ifndef JCM_EVAL_H
#define JCM_EVAL_H

#include "ast.h"

/*
 * JCMValueKind is defined in core.h.
 */

struct JCMEnv;
struct JCMFunction;

struct JCMValue {
    enum JCMValueKind kind;

    union {
        long number;
        struct JCMFunction *function;
    } value;
};

struct JCMBinding {
    char *name;
    struct JCMValue value;
    struct JCMBinding *next;
};

struct JCMEnv {
    struct JCMBinding *bindings;
    struct JCMEnv *parent;
};

struct JCMFunction {
    const struct JCMAst *lambda;
    struct JCMEnv *env;
};

struct JCMMemoryCell {
    long address;
    struct JCMValue value;
    struct JCMMemoryCell *next;
};

struct JCMMemory {
    struct JCMMemoryCell *cells;
    long next_address;
};

struct JCMRuntime {
    struct JCMEnv *global;
    struct JCMMemory memory;
};

struct JCMEnv *
jcm_env_new(struct JCMEnv *parent);

void
jcm_env_free(struct JCMEnv *env);

int
jcm_env_define(
    struct JCMEnv *env,
    const char *name,
    struct JCMValue value
);

int
jcm_env_set(
    struct JCMEnv *env,
    const char *name,
    struct JCMValue value
);

int
jcm_env_get(
    struct JCMEnv *env,
    const char *name,
    struct JCMValue *value
);

struct JCMValue
jcm_value_number(long number);

struct JCMValue
jcm_value_function(struct JCMFunction *function);

int
jcm_runtime_init(struct JCMRuntime *runtime);

void
jcm_runtime_free(struct JCMRuntime *runtime);

int
jcm_eval(
    struct JCMRuntime *runtime,
    struct JCMEnv *env,
    const struct JCMAst *ast,
    struct JCMValue *result
);

int
jcm_eval_program(
    struct JCMRuntime *runtime,
    const struct JCMAst *program,
    struct JCMValue *result
);

int
jcm_memory_load(
    struct JCMMemory *memory,
    long address,
    struct JCMValue *value
);

int
jcm_memory_store(
    struct JCMMemory *memory,
    long address,
    struct JCMValue value
);

void
jcm_value_print(struct JCMValue value);

#endif

