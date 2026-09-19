# JCM

JCM (John McCarthy) is a minimal Lisp language using Polish
notation for computation, memory, I/O, and native compilation.

# CORE

Logic:

```
(∧ A B)
(∨ A B)
(¬ A)
```

Comparison:

```
(= A B)
(< A B)
(≤ A B)
(> A B)
(≥ A B)
```

Arithmetic:

```
(+ A B)
(- A B)
(* A B)
(/ A B)
(% A B)
```

Control:

```
(if C T E)
```

Functions:

```
(λ (X) E)
(X : E)
```

Memory:

```
(← A)
(→ A V)
```

I/O:

```
(↑)
(↓ V)
```

# VALUES

Numbers are machine values.

Boolean results are:

```
0
1
```

0 is false. 1 is true.

# SYNTAX

JCM uses Polish notation:

```
(+ 2 (* 3 4))
```

Comments begin with `;`:

```
; comment
```

Expressions may be nested arbitrarily.

# SEMANTICS

`if` evaluates only the selected branch.

`λ` creates a function.

`:` binds a name to an expression. Bindings may be recursive.

`←` reads memory.

`→` writes memory.

`↑` reads from stdin.

`↓` writes to stdout.

Comparison operations return `0` or `1`.

Arithmetic operates on machine values.

# COMPUTATION

JCM provides conditional evaluation, functions, recursion, mutable
memory, arithmetic, and comparison.

These mechanisms provide general computation.

Higher-level operations such as loops, NAND, NOR, and XOR can be
defined using the core language.

# EXAMPLE

```
; Hello World

(↓ 72)
(↓ 101)
(↓ 108)
(↓ 108)
(↓ 111)
(↓ 32)
(↓ 87)
(↓ 111)
(↓ 114)
(↓ 108)
(↓ 100)
(↓ 10)
```

# FILES

Source files use `.jcm`.

Reference implementation: C99.

Target: x86_64 Linux.

# LICENSE

See LICENSE.

