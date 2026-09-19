#include "test_support.h"

int
main(void)
{
    expect_program_result("((λ (x) (+ x 3)) 4)", 7);
    expect_program_result("(x : 4) ((λ (y) (+ x y)) 3)", 7);

    return 0;
}
