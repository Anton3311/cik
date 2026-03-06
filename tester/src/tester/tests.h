#ifndef TESTS_H
#define TESTS_H

#include "tester/tester_core.h"

void test_text_start_position_to_source_location(TestContext* context);
void test_last_line_postion_to_source_location(TestContext* context);

void test_token_has_valid_string_represenation(TestContext* context);
void test_token_source_range_matches_token_string(TestContext* context);
void test_tokenizer_generates_expected_token(TestContext* context);

void test_non_function_style_macro_expansion(TestContext* context);
void test_expand_function_style_macro_with_two_params(TestContext* context);
void test_expand_function_style_macro_without_params(TestContext* context);
void test_expand_empty_function_style_macro(TestContext* context);
void test_expand_empty_style_macro(TestContext* context);
void test_macro_call_with_not_enough_args_fails(TestContext* context);
void test_nested_macro(TestContext* context);
void test_nested_macro_2(TestContext* context);
void test_builtin_line_macro_expantion(TestContext* context);
void test_assert_macro_expantion(TestContext* context);

void test_macro_string_operator(TestContext* context);
void test_macro_string_operator_with_invalid_param_name_fails(TestContext* context);

void test_parse_type_def_of_primitive_type(TestContext* context);
void test_parse_type_def_of_struct_def(TestContext* context);
void test_parse_type_def_of_struct_def_with_members(TestContext* context);
void test_parse_enum_def(TestContext* context);

#endif
