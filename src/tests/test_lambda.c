#include "test_support.h"

int
main(void)
{
    expect_input_result("(↑)", "42\n", 42);
    expect_output_result("(↓ 65)", 65, "A");

    return 0;
}
