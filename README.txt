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
(↑)       ; read a number from stdin
(↓ V)     ; print a number in decimal
(print V) ; convert a number to one output character
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

`↑` reads a decimal number from stdin.

`↓` writes a number in decimal form.

`print` converts a numeric value to one byte/character using `putchar`.
For example, `(↓ 99)` prints `99`, while `(print 99)` prints `c`.

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
(print 72)
(print 101)
(print 108)
(print 108)
(print 111)
(print 32)
(print 87)
(print 111)
(print 114)
(print 108)
(print 100)
(print 10)
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

The first example prints "Hello World". The factorial example prints
`120`. The memory demo prints `12`. The input example prints `even? 0`
for input `99`, and `even? 1` for input `100`.

These examples collectively exercise the full language surface: logic,
comparison, arithmetic, control flow, functions, binding, memory, and I/O.

# FILES

Source files use `.jcm`.

Reference implementation: C99.

Target: x86_64 Linux.

# LICENSE

See LICENSE.
