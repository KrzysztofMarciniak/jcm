#include "test_support.h"

int
main(void)
{
    expect_program_result("(x : 5) x", 5);
    expect_program_result("(: y 9) y", 9);
    expect_program_result(
        "(fact : (λ (n) (if (= n 0) 1 (* n (fact (- n 1)))))) (fact 5)",
        120
    );

    return 0;
}
