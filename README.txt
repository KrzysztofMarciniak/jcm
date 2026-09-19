                              J C M
                  	   ((J)ohn Mc(C)arthy (M)inimal lisp) 

JCM is a small Lisp and Turing-machine language.  It uses Polish
notation for computation, memory, input, output, and native compilation.

Motivation:

JCM was designed around a simple constraint: the language should be small enough to write and reason about entirely with pen and paper. Mathematics and pseudocode provide useful tools for describing computation, but I found neither sufficiently expressive and concise at the same time. JCM is my attempt to bridge that gap: to provide a usable programming language with simple abstractions for memory, functions, and I/O, while keeping the language tiny enough to reason about without electronics.

For example, working with dynamically allocated memory in C can involve explicit allocation, pointers, and deallocation:

int *memory = malloc(2 * sizeof(int));
if (memory == NULL) return 1;
memory[0] = 3;
memory[1] = memory[0] + 9;
printf("%d\n", memory[1]);
free(memory);

In JCM, the same computation can be expressed using its simple memory model:

(→ 7003 3) ; on address 7003, save value 3
(→ 7009 (+ (← 7003) 9)) ; On address 7009, save solution to 7003(3) + 9
(↓ (← 7009)) ; print the value stored at address 7009(12)

The point is not that other notations are objectively bad. JCM is simply the representation that is best suited to how I think about programs. Different programmers may naturally prefer different representations and syntax, and I would encourage others to experiment with the idea and implement their own.


John McCarthy:

    “He who refuses to do arithmetic is doomed to talk nonsense.”


-----------------------------------------------------------------------
                                CONTENTS
-----------------------------------------------------------------------

    1.  Core operations
    2.  Values and syntax
    3.  Small examples
    4.  Functions and recursion
    5.  Memory and input
    6.  Complete programs
    7.  Building and running


-----------------------------------------------------------------------
                            1. CORE OPERATIONS
-----------------------------------------------------------------------

LOGIC

    (∧ A B)                 logical AND
    (∨ A B)                 logical OR
    (¬ A)                   logical NOT

COMPARISON

    (= A B)                 equal
    (< A B)                 less than
    (≤ A B)                 less than or equal
    (> A B)                 greater than
    (≥ A B)                 greater than or equal

ARITHMETIC

    (+ A B)                 addition
    (- A B)                 subtraction
    (* A B)                 multiplication
    (/ A B)                 division
    (% A B)                 remainder

CONTROL

    (if C T E)              evaluate T when C is true, otherwise E

FUNCTIONS AND BINDINGS

    (λ (X) E)               create a function with parameter X
    (X : E)                 bind the expression E to the name X

MEMORY

    (← A)                   read the value at address A
    (→ A V)                 write V to address A

INPUT AND OUTPUT

    (↑)                     read a decimal number from standard input
    (↓ V)                   print a number or a string

SOURCE INCLUDES

    #include "filename"    include another JCM source file


-----------------------------------------------------------------------
                         2. VALUES AND SYNTAX
-----------------------------------------------------------------------

Numbers are machine values.  Boolean results are represented by numbers:

    0                       false
    1                       true

JCM uses Polish notation: the operation comes before its arguments.
Nested expressions are written from the inside out when they are read:

    (+ 2 (* 3 4))           2 + (3 * 4), which produces 14

Comments begin with a semicolon and continue to the end of the line:

    ; this entire line is a comment
    (+ 2 3)                  ; comments may follow an expression

Include directives are handled before parsing, so they can appear at the top
of a file and expand to the contents of another JCM source file.

Strings are enclosed in single quotes.  Escaped characters are allowed:

    (↓ 'hello world!')
    (↓ 'first line\nsecond line')

An expression can be placed anywhere a value is expected:

    (↓ (+ (* 3 4) (- 10 2)))

The conditional evaluates only the branch selected by its condition.
A lambda creates a function.  A binding gives an expression a name and
can be recursive.  Comparisons and logic return 0 or 1.


-----------------------------------------------------------------------
                            3. SMALL EXAMPLES
-----------------------------------------------------------------------

HELLO, WORLD

    (↓ 'Hello, world!')

    Output:
        Hello, world!

ARITHMETIC

    (+ 2 3)                  ; 5
    (- 9 4)                  ; 5
    (* 6 7)                  ; 42
    (/ 20 4)                 ; 5
    (% 10 3)                 ; 1

NESTED ARITHMETIC

    (+ 1 (* 2 3))            ; 7
    (* (+ 2 3) (- 9 4))      ; 25
    (/ (+ 10 5) 3)           ; 5
    (% (* 4 7) 5)             ; 3

COMPARISONS

    (= 5 5)                  ; 1
    (= 5 6)                  ; 0
    (< 2 3)                  ; 1
    (≤ 4 4)                  ; 1
    (> 9 7)                  ; 1
    (≥ 8 8)                  ; 1

LOGIC

    (∧ 1 1)                  ; 1
    (∧ 1 0)                  ; 0
    (∨ 0 1)                  ; 1
    (∨ 0 0)                  ; 0
    (¬ 0)                    ; 1
    (¬ (= 2 3))              ; 1

CONDITIONALS

    (if (= 3 3) 42 99)       ; 42
    (if (< 5 2) 1 0)         ; 0
    (if (∧ 1 1) (+ 2 3) 10)  ; 5

BINDINGS

    (x : 5)
    (+ x 2)                  ; 7

    (width  : 6)
    (height : 7)
    (* width height)          ; 42

A binding can contain another expression:

    (answer : (+ (* 6 7) 1))
    (↓ answer)               ; 43


-----------------------------------------------------------------------
                        4. FUNCTIONS AND RECURSION
-----------------------------------------------------------------------

ANONYMOUS FUNCTIONS

    ((λ (x) (+ x 3)) 4)      ; 7
    ((λ (x) (* x x)) 9)      ; 81
    ((λ (a) (- a 1)) 10)     ; 9

NAMED FUNCTIONS

    (square : (λ (n) (* n n)))
    (square 9)                ; 81

    (double : (λ (n) (+ n n)))
    (double 21)               ; 42

FUNCTIONS WITH MORE THAN ONE VALUE

    (add : (λ (a) (λ (b) (+ a b))))
    ((add 20) 22)             ; 42

A function can be used inside a larger expression:

    (triple : (λ (n) (* n 3)))
    (+ (triple 4) (triple 10)) ; 42

CLOSURES

    (make-adder : (λ (x) (λ (y) (+ x y))))
    (add-five   : (make-adder 5))
    (add-five 37)              ; 42

FACTORIAL

    (fact : (λ (n)
        (if (= n 0)
            1
            (* n (fact (- n 1))))))

    (fact 0)                  ; 1
    (fact 5)                  ; 120
    (fact 6)                  ; 720

FIBONACCI

    (fib : (λ (n)
        (if (< n 2)
            n
            (+ (fib (- n 1))
               (fib (- n 2))))))

    (fib 0)                   ; 0
    (fib 1)                   ; 1
    (fib 10)                  ; 55

EVENNESS

    (is-even : (λ (n) (= (% n 2) 0)))

    (is-even 14)               ; 1
    (is-even 15)               ; 0

ABSOLUTE VALUE

    (absolute : (λ (n)
        (if (< n 0)
            (- 0 n)
            n)))

    (absolute (- 3 8))        ; 5
    (absolute 12)              ; 12


-----------------------------------------------------------------------
                         5. MEMORY AND INPUT
-----------------------------------------------------------------------

MEMORY CELLS

    (→ 7 3)                    ; store 3 at address 7
    (← 7)                      ; 3

    (→ 8 (+ (← 7) 9))          ; store 12 at address 8
    (↓ (← 8))                  ; print 12

A later write replaces the previous value:

    (→ 5 12)
    (→ 5 18)
    (↓ (← 5))                  ; print 18

MEMORY CAN HOLD INTERMEDIATE RESULTS

    (→ 0 10)
    (→ 1 20)
    (→ 2 (+ (← 0) (← 1)))
    (↓ (← 2))                  ; print 30

COUNTER-STYLE UPDATE

    (→ 100 0)
    (→ 100 (+ (← 100) 1))
    (→ 100 (+ (← 100) 1))
    (↓ (← 100))                ; print 2

READING INPUT

    (number : (↑))
    (↓ number)

For input 42, the program above prints 42.

INPUT WITH A CONDITIONAL

    (number : (↑))
    (if (≥ number 0)
        (↓ 'non-negative')
        (↓ 'negative'))

INPUT AND ARITHMETIC

    (number : (↑))
    (double : (λ (n) (* n 2)))
    (↓ (double number))

For input 21, the program prints 42.


-----------------------------------------------------------------------
                         6. COMPLETE PROGRAMS
-----------------------------------------------------------------------

The following examples can be saved as .jcm files.

GREETING PROGRAM

    (↓ 'Welcome to JCM!')
    (↓ 'A tiny language with big ideas.')

CALCULATOR PROGRAM

    (left  : 6)
    (right : 7)
    (↓ (* left right))

TEMPERATURE CHECK

    (temperature : (↑))
    (if (< temperature 0)
        (↓ 'freezing')
        (if (< temperature 20)
            (↓ 'cold')
            (↓ 'warm')))

SUM FROM ONE THROUGH N

    (sum-to : (λ (n)
        (if (= n 0)
            0
            (+ n (sum-to (- n 1))))))

    (↓ (sum-to 10))            ; 55

GREATEST COMMON DIVISOR

    (gcd : (λ (a)
        (λ (b)
            (if (= b 0)
                a
                ((gcd b) (% a b))))))

    (↓ ((gcd 48) 18))           ; 6

MEMORY-BACKED TOTAL

    (→ 10 12)
    (→ 11 30)
    (total : (+ (← 10) (← 11)))
    (↓ total)                   ; 42

A SMALL REPORT

    (name : 'JCM')
    (version : 1)
    (↓ name)
    (↓ ' version ')
    (↓ version)


-----------------------------------------------------------------------
                         7. BUILDING AND RUNNING
-----------------------------------------------------------------------

Build the interpreter with:

    make

Run source files with:

    ./jcm --eval examples/hello-world.jcm
    ./jcm --eval examples/factorial.jcm
    ./jcm --eval examples/memory-demo.jcm
    ./jcm --eval examples/include1.jcm

Provide decimal input through standard input:

    printf '99\n' | ./jcm --eval examples/input-check.jcm

The source-file extension is .jcm.

Include directives work in source files before parsing:

    #include "include2.jcm"

The reference implementation is written in C99.
The current target is x86_64 Linux.


-----------------------------------------------------------------------
                               LICENSE
-----------------------------------------------------------------------

See LICENSE.
