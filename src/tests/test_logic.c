#include "test_support.h"

int
main(void)
{
    expect_program_result("(∧ 1 1)", 1);
    expect_program_result("(∧ 1 0)", 0);
    expect_program_result("(∨ 0 1)", 1);
    expect_program_result("(∨ 0 0)", 0);
    expect_program_result("(¬ 0)", 1);
    expect_program_result("(¬ 1)", 0);

    return 0;
}
