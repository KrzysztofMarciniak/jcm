#include "test_support.h"

int
main(void)
{
    expect_program_result("(+ 2 3)", 5);
    expect_program_result("(- 9 4)", 5);
    expect_program_result("(* 6 7)", 42);
    expect_program_result("(/ 22 4)", 5);
    expect_program_result("(% 17 5)", 2);

    return 0;
}
