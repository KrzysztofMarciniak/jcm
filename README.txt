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
(↑)      ; read a decimal number from stdin
(↓ V)    ; print a number or string
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

Strings use `'` and may contain escaped characters such as `\n`:

```
(↓ 'hello world')
(↓ 'line one\nline two')
```

Expressions may be nested arbitrarily.

# SEMANTICS

`if` evaluates only the selected branch.

`λ` creates a function.

`:` binds a name to an expression. Bindings may be recursive.

`←` reads memory.

`→` writes memory.

`↑` reads a decimal number from stdin.

`↓` prints either a number or a string literal.

Comparison operations return `0` or `1`.

Arithmetic operates on machine values.

# COMPUTATION

JCM provides conditional evaluation, functions, recursion, mutable
memory, arithmetic, comparison, and string output.

These mechanisms provide general computation.

# EXAMPLE

```
(↓ 'Hello, world!')
(↓ 10)
```

# RUNNING EXAMPLES

After building the interpreter:

```
make
./jcm --eval examples/hello-world.jcm
./jcm --eval examples/factorial.jcm
./jcm --eval examples/memory-demo.jcm
printf '99\n' | ./jcm --eval examples/input-check.jcm
```

The first example prints `Hello, world!`. The factorial example prints
`120`. The memory demo prints `12`. The input example prints the evenness
check for the supplied number.

# FILES

Source files use `.jcm`.

Reference implementation: C99.

Target: x86_64 Linux.

# LICENSE

See LICENSE.
