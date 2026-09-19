#ifndef JCM_CORE_H
#define JCM_CORE_H

#include <string.h>

enum JCMCategory {
    JCM_LOGIC,
    JCM_COMPARISON,
    JCM_ARITHMETIC,
    JCM_CONTROL,
    JCM_FUNCTION,
    JCM_BINDING,
    JCM_MEMORY,
    JCM_IO
};

enum JCMFormKind {
    JCM_PRIMITIVE,
    JCM_SPECIAL_FORM
};

enum JCMValueKind {
    JCM_VALUE_NUMBER,
    JCM_VALUE_BOOLEAN,
    JCM_VALUE_FUNCTION
};

#define JCM_MACHINE_BITS 64
#define JCM_FALSE 0
#define JCM_TRUE  1

enum JCMOp {
    JCM_AND,
    JCM_OR,
    JCM_NOT,
    JCM_EQ,
    JCM_LT,
    JCM_LE,
    JCM_GT,
    JCM_GE,
    JCM_ADD,
    JCM_SUB,
    JCM_MUL,
    JCM_DIV,
    JCM_MOD,
    JCM_IF,
    JCM_LAMBDA,
    JCM_BIND,
    JCM_LOAD,
    JCM_STORE,
    JCM_INPUT,
    JCM_OUTPUT,
    JCM_PRINT
};

struct JCMCoreForm {
    enum JCMOp op;
    enum JCMCategory category;
    enum JCMFormKind kind;
    const char *name;
    const char *symbol;
    int min_args;
    int max_args;
    const char *syntax;
    int returns_boolean;
    int special_evaluation;
};

static const struct JCMCoreForm jcm_core[] = {
    { JCM_AND, JCM_LOGIC, JCM_PRIMITIVE, "and", "∧", 2, 2, "(∧ A B)", 1, 0 },
    { JCM_OR, JCM_LOGIC, JCM_PRIMITIVE, "or", "∨", 2, 2, "(∨ A B)", 1, 0 },
    { JCM_NOT, JCM_LOGIC, JCM_PRIMITIVE, "not", "¬", 1, 1, "(¬ A)", 1, 0 },
    { JCM_EQ, JCM_COMPARISON, JCM_PRIMITIVE, "equal", "=", 2, 2, "(= A B)", 1, 0 },
    { JCM_LT, JCM_COMPARISON, JCM_PRIMITIVE, "less", "<", 2, 2, "(< A B)", 1, 0 },
    { JCM_LE, JCM_COMPARISON, JCM_PRIMITIVE, "less_equal", "≤", 2, 2, "(≤ A B)", 1, 0 },
    { JCM_GT, JCM_COMPARISON, JCM_PRIMITIVE, "greater", ">", 2, 2, "(> A B)", 1, 0 },
    { JCM_GE, JCM_COMPARISON, JCM_PRIMITIVE, "greater_equal", "≥", 2, 2, "(≥ A B)", 1, 0 },
    { JCM_ADD, JCM_ARITHMETIC, JCM_PRIMITIVE, "add", "+", 2, 2, "(+ A B)", 0, 0 },
    { JCM_SUB, JCM_ARITHMETIC, JCM_PRIMITIVE, "subtract", "-", 2, 2, "(- A B)", 0, 0 },
    { JCM_MUL, JCM_ARITHMETIC, JCM_PRIMITIVE, "multiply", "*", 2, 2, "(* A B)", 0, 0 },
    { JCM_DIV, JCM_ARITHMETIC, JCM_PRIMITIVE, "divide", "/", 2, 2, "(/ A B)", 0, 0 },
    { JCM_MOD, JCM_ARITHMETIC, JCM_PRIMITIVE, "modulo", "%", 2, 2, "(% A B)", 0, 0 },
    { JCM_IF, JCM_CONTROL, JCM_SPECIAL_FORM, "if", "if", 3, 3, "(if C T E)", 0, 1 },
    { JCM_LAMBDA, JCM_FUNCTION, JCM_SPECIAL_FORM, "lambda", "λ", 2, 2, "(λ (X) E)", 0, 1 },
    { JCM_BIND, JCM_BINDING, JCM_SPECIAL_FORM, "bind", ":", 2, 2, "(X : E)", 0, 1 },
    { JCM_LOAD, JCM_MEMORY, JCM_PRIMITIVE, "load", "←", 1, 1, "(← A)", 0, 0 },
    { JCM_STORE, JCM_MEMORY, JCM_PRIMITIVE, "store", "→", 2, 2, "(→ A V)", 0, 0 },
    { JCM_INPUT, JCM_IO, JCM_PRIMITIVE, "input", "↑", 0, 0, "(↑)", 0, 0 },
    { JCM_OUTPUT, JCM_IO, JCM_PRIMITIVE, "output", "↓", 1, 1, "(↓ V)", 0, 0 },
    { JCM_PRINT, JCM_IO, JCM_PRIMITIVE, "print", "print", 1, 1, "(print V)", 0, 0 }
};

#define JCM_CORE_COUNT ((int)(sizeof(jcm_core) / sizeof(jcm_core[0])))
#define JCM_COMMENT_CHAR ';'
#define JCM_LPAREN '('
#define JCM_RPAREN ')'
#define JCM_SOURCE_EXTENSION ".jcm"
#define JCM_BOOLEAN_MIN JCM_FALSE
#define JCM_BOOLEAN_MAX JCM_TRUE
#define JCM_BOOLEAN_RESULT(op) \
    ((op) == JCM_AND || (op) == JCM_OR || (op) == JCM_NOT || \
     (op) == JCM_EQ || (op) == JCM_LT || (op) == JCM_LE || \
     (op) == JCM_GT || (op) == JCM_GE)

#if defined(__GNUC__) || defined(__clang__)
#define JCM_CORE_UNUSED __attribute__((unused))
#else
#define JCM_CORE_UNUSED
#endif

static const struct JCMCoreForm * JCM_CORE_UNUSED
jcm_core_find(const char *symbol)
{
    int i;
    for (i = 0; i < JCM_CORE_COUNT; i++) {
        if (strcmp(jcm_core[i].symbol, symbol) == 0)
            return &jcm_core[i];
    }
    return NULL;
}

static const struct JCMCoreForm * JCM_CORE_UNUSED
jcm_core_find_op(enum JCMOp op)
{
    int i;
    for (i = 0; i < JCM_CORE_COUNT; i++) {
        if (jcm_core[i].op == op)
            return &jcm_core[i];
    }
    return NULL;
}

#undef JCM_CORE_UNUSED

#endif
