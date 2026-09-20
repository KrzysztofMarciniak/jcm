# Error examples

These files are intentionally invalid JCM programs. Run one at a time to see
location-aware compiler and evaluator diagnostics:

```sh
./jcm --eval examples/errors/wrong-arity-add.jcm
./jcm --eval examples/errors/wrong-arity-if.jcm
./jcm --eval examples/errors/wrong-arity-output.jcm
./jcm --eval examples/errors/undefined-symbol.jcm
./jcm --eval examples/errors/type-error.jcm
./jcm --eval examples/errors/unclosed-list.jcm
./jcm --eval examples/errors/unclosed-string.jcm
./jcm --eval examples/errors/number-overflow.jcm
```

All examples in this directory are expected to fail; they are documentation
and manual diagnostic checks, not normal programs to run as part of `make test`.

Expected diagnostic categories include:

- incorrect argument count, with the correct core-form definition;
- undefined symbols and invalid operand types;
- unclosed lists and strings;
- integer literals outside the supported range.
