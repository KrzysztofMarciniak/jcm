#include "test_support.h"

int
main(void)
{
    expect_program_result("(= 5 5)", 1);
    expect_program_result("(= 5 6)", 0);
    expect_program_result("(< 3 7)", 1);
    expect_program_result("(≤ 7 7)", 1);
    expect_program_result(">( 8 2)", 1);
    expect_program_result("(≥ 9 9)", 1);

    return 0;
}
