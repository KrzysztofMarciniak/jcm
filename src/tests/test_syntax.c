#include "test_support.h"

int
main(void)
{
    expect_program_result("; comment\n(+ (* 2 3) (- 9 4))", 11);
    expect_program_result("(+ 2 (* 3 (+ 4 1)))", 17);

    return 0;
}
