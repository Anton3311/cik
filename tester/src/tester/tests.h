#ifndef TESTS_H
#define TESTS_H

#include "tester/tester_core.h"

void test_text_start_position_to_source_location(TestContext* context);
void test_last_line_postion_to_source_location(TestContext* context);

void test_non_function_style_macro_expansion(TestContext* context);
void test_fail(TestContext* context);
void test_success(TestContext* context);

#endif
