#include "test_support.h"

int
main(void)
{
    expect_input_result("(↑)", "42\n", 42);
    expect_output_result("(↓ 65)", 65, "65");
    expect_string_output("(↓ 'Hello')", "Hello");

    /* The source contains the two characters backslash+n. */
    expect_string_output("(↓ '\\n')", "\n");

    return 0;
}
