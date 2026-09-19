#include "test_support.h"

int main(void)
{
    expect_program_result("(∑ i 1 5 i)", 15);
    expect_program_result("(∑ i 1 5 (* i i))", 55);
    expect_program_result("(∑ i 5 1 i)", 0);
    expect_program_result("(∏ i 1 5 i)", 120);
    expect_program_result("(∏ i 5 1 i)", 1);
    expect_program_result("(∫ (λ (x) (* x x)) 0 10 10)", 335);
    return 0;
}
