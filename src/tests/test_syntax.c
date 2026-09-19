#include "test_support.h"

int
main(void)
{
    expect_program_result("(x : 5) (+ x 2)", 7);
    expect_program_result("(n : 3) (m : 4) (+ n m)", 7);
    expect_program_result("((λ (x) (+ x 3)) 4)", 7);

    return 0;
}
