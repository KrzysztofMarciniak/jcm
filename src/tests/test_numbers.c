#include "test_support.h"

int
main(void)
{
    expect_program_result("(→ 5 12) (← 5)", 12);
    expect_program_result("(→ 5 12) (→ 5 18) (← 5)", 18);
    expect_program_failure("(← 999)");

    return 0;
}
