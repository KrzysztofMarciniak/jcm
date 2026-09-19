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

# MORE EXAMPLES

Basic arithmetic:

```
(+ 2 3)                  ; 5
(- 9 4)                  ; 5
(* 6 7)                  ; 42
(/ 20 4)                 ; 5
(% 10 3)                 ; 1
```

Nested arithmetic:

```
(+ 1 (* 2 3))            ; 7
(* (+ 2 3) (- 9 4))      ; 25
(/ (+ 10 5) 3)           ; 5
```

Comparisons and logic:

```
(= 5 5)                  ; 1
(< 2 3)                  ; 1
(≤ 4 4)                  ; 1
(> 9 7)                  ; 1
(≥ 8 8)                  ; 1
(∧ 1 0)                  ; 0
(∨ 0 1)                  ; 1
(¬ (= 2 3))              ; 1
```

Conditionals:

```
(if (= 3 3) 42 99)       ; 42
(if (< 5 2) 1 0)         ; 0
(if (∧ 1 1) (+ 2 3) 10) ; 5
```

Bindings and names:

```
(x : 5)                  ; bind x = 5
(+ x 2)                  ; 7
(y : (+ 3 4))            ; bind y = 7
(* y 2)                  ; 14
```

Closures and lambda expressions:

```
((λ (x) (+ x 3)) 4)      ; 7
((λ (x) (λ (y) (+ x y))) 2 3)   ; 5
```

A slightly larger function example:

```
(square : (λ (n) (* n n)))
(square 9)               ; 81
```

Factorial with recursion:

```
(fact : (λ (n) (if (= n 0) 1 (* n (fact (- n 1))))))
(fact 5)                 ; 120
```

Memory operations:

```
(→ 7 3)                  ; store 3 at address 7
(← 7)                    ; 3
(→ 8 (+ (← 7) 9))        ; store 12 at address 8
(↓ (← 8))                ; prints 12
```

Multiple memory cells:

```
(→ 0 10)
(→ 1 20)
(+ (← 0) (← 1))          ; 30
```

String output:

```
(↓ 'Hello, world!')      ; prints Hello, world!
(↓ 'JCM rocks!')         ; prints JCM rocks!
(↓ 'line one\nline two') ; prints two lines
```

Printing mixed values:

```
(↓ 10)                   ; prints 10
(↓ 'answer:')            ; prints answer:
(↓ (+ 5 7))              ; prints 12
```

A complete mini program:

```
; compute and print the value of a simple expression
(↓ (+ (* 3 4) 5))        ; prints 17
```

Another small program:

```
; definition and use of a named function
(sum2 : (λ (x) (+ x 2)))
(↓ (sum2 10))            ; prints 12
```

A boolean program:

```
; check whether a value is even
(is_even : (λ (n) (= (% n 2) 0)))
(↓ (is_even 14))         ; prints 1
(↓ (is_even 15))         ; prints 0
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
