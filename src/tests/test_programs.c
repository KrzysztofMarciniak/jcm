#include "test_support.h"

int
main(void)
{
    expect_program_result("5", 5);
    expect_program_result("-42", -42);
    expect_program_result("+7", 7);
    expect_program_result("0", 0);

    return 0;
}
