# JCM

JCM (John McCarthy) is a minimal Lisp language using Polish notation for computation, memory, I/O, and native compilation.

## Core forms

Arithmetic includes `+`, `-`, `*`, `/`, and `%`. Mathematical iteration forms use Unicode symbols:

```lisp
(∑ I START END BODY)       ; inclusive integer summation
(∏ I START END BODY)       ; inclusive integer product
(∫ F START END STEPS)      ; numerical integral using trapezoids
```

Examples:

```sh
./jcm --eval examples/summation.jcm
./jcm --eval examples/product.jcm
./jcm --eval examples/integral.jcm
./jcm --eval examples/calculus.jcm
```

```lisp
(∑ i 1 5 i)                         ; 15
(∑ i 1 5 (* i i))                   ; 55
(∏ i 1 5 i)                         ; 120
(∫ (λ (x) (* x x)) 0 10 10)         ; 335 (integer result)
```

`∑` and `∏` bind their first argument for each integer in the inclusive range. A reversed range returns the identity (`0` for summation and `1` for product). `∫` takes a one-argument function, integer bounds, and a positive number of trapezoidal steps. Since JCM currently has integer values, sample points are rounded by integer arithmetic and the final result is an integer.

## Running

```sh
make
make test
./jcm --eval examples/hello-world.jcm
```

See `LICENSE` for licensing information.
