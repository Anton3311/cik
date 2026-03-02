#ifndef TESTS_H
#define TESTS_H

#include "tester/tester_core.h"

void test_text_start_position_to_source_location(TestContext* context);
void test_last_line_postion_to_source_location(TestContext* context);

void test_non_function_style_macro_expansion(TestContext* context);
void test_expand_function_style_macro_with_no_params(TestContext* context);

#endif
