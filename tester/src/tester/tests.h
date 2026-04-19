#ifndef TESTS_H
#define TESTS_H

#include "tester/tester_core.h"

void test_text_start_position_to_source_location(TestContext* context);
void test_last_line_postion_to_source_location(TestContext* context);
void test_line_range_of_one_line_source_code(TestContext* context);



void test_token_has_valid_string_represenation(TestContext* context);
void test_token_source_range_matches_token_string(TestContext* context);
void test_tokenizer_generates_expected_token(TestContext* context);
void test_mutli_line_comment_with_asterisk_on_line_starts(TestContext* context);
void test_token_has_valid_source_file(TestContext* context);



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

void test_simple_if_elif_directives(TestContext* context);
void test_parsing_of_non_function_style_macro_with_paren_as_first_token_in_stream(TestContext* context);
void test_error_directive(TestContext* context);
void test_multi_line_define(TestContext* context);
void test_token_insertion_operator(TestContext* context);
void test_va_args_macro(TestContext* context);
void test_processor_next_returns_eof_when_include_stack_is_empty(TestContext* context);



void test_parse_type_def_of_primitive_type(TestContext* context);
void test_parse_type_def_of_struct_def(TestContext* context);
void test_parse_type_def_of_struct_def_with_members(TestContext* context);
void test_aliased_type_resolution(TestContext* context);

void test_parse_enum_def(TestContext* context);
void test_parse_function_def(TestContext* context);

void test_parse_forward_declared_struct(TestContext* context);
void test_parse_forward_declared_struct_followed_by_definition(TestContext* context);

void test_parse_forward_declared_enum(TestContext* context);
void test_parse_forward_declared_enum_followed_by_definition(TestContext* context);

void test_parse_forward_declared_function(TestContext* context);
void test_parse_forward_declared_function_followed_by_definition(TestContext* context);

void test_parse_function_ref_expr(TestContext* context);
void test_parse_primitive_integer_types(TestContext* context);

void test_parse_variable_declaration(TestContext* context);
void test_parse_simple_bin_expr(TestContext* context);
void test_bin_op_precedence(TestContext* context);
void test_parse_variable_ref_expr(TestContext* context);

void test_parse_return_stmt(TestContext* context);
void test_parse_return_stmt_without_value(TestContext* context);
void test_multi_part_string_merging(TestContext* context);
void test_parse_expr_inside_parens(TestContext* context);

void test_allow_variable_shadowing_in_nested_blocks(TestContext* context);

#endif
