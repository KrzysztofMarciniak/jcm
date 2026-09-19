#include "test_support.h"

int
main(void)
{
    expect_program_result("(if 1 7 9)", 7);
    expect_program_result("(if 0 7 9)", 9);
    expect_program_result("(if 0 (/ 1 0) 5)", 5);

    return 0;
}
