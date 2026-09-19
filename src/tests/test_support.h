#ifndef JCM_TEST_SUPPORT_H
#define JCM_TEST_SUPPORT_H

void expect_program_result(const char *source, long expected);
void expect_program_failure(const char *source);
void expect_input_result(const char *source, const char *input_text, long expected);
void expect_output_result(const char *source, long expected, const char *output);
void expect_string_output(const char *source, const char *output);

#endif
