#include <stdio.h>

#include "core/core.h"

#include "tester/tester_core.h"
#include "tester/tests.h"
#include "tester/tests_x64.h"
#include "tester/tests_compiler.h"

#include "parser/preprocessor.h"

static TestCase source_info_tests[] = {
	test(test_text_start_position_to_source_location),
	test(test_last_line_postion_to_source_location),
	test(test_line_range_of_one_line_source_code),
};

static TestCase tokenizer_tests[] = { 
	test(test_tokenizer_generates_expected_token),
	test(test_token_has_valid_string_represenation),
	test(test_token_source_range_matches_token_string),
	test(test_mutli_line_comment_with_asterisk_on_line_starts),
	test(test_token_has_valid_source_file),
};

static TestCase preprocessor_tests[] = {
	test(test_non_function_style_macro_expansion),
	test(test_expand_function_style_macro_with_two_params),
	test(test_expand_function_style_macro_without_params),
	test(test_expand_empty_function_style_macro),
	test(test_expand_empty_style_macro),
	test(test_macro_call_with_not_enough_args_fails),
	test(test_nested_macro),
	test(test_nested_macro_2),
	test(test_builtin_line_macro_expantion),
	test(test_assert_macro_expantion),
	test(test_macro_string_operator),
	test(test_macro_string_operator_with_invalid_param_name_fails),
	test(test_simple_if_elif_directives),
	test(test_parsing_of_non_function_style_macro_with_paren_as_first_token_in_stream),
	test(test_error_directive),
	test(test_multi_line_define),
	test(test_token_insertion_operator),
	test(test_va_args_macro),
	test(test_processor_next_returns_eof_when_include_stack_is_empty)
};

static TestCase parser_tests[] = {
	test(test_parse_type_def_of_primitive_type),
	test(test_parse_type_def_of_struct_def),
	test(test_parse_type_def_of_struct_def_with_fields),
	test(test_aliased_type_resolution),
	test(test_parse_enum_def),
	test(test_parse_function_def),
	test(test_parse_forward_declared_struct),
	test(test_parse_forward_declared_struct_followed_by_definition),
	test(test_parse_forward_declared_enum),
	test(test_parse_forward_declared_enum_followed_by_definition),
	test(test_parse_forward_declared_function),
	test(test_parse_forward_declared_function_followed_by_definition),
	test(test_parse_function_ref_expr),
	test(test_parse_primitive_integer_types),
	test(test_parse_variable_declaration),
	test(test_parse_simple_bin_expr),
	test(test_bin_op_precedence),
	test(test_parse_variable_ref_expr),
	test(test_parse_return_stmt),
	test(test_parse_return_stmt_without_value),
	test(test_multi_part_string_merging),
	test(test_parse_expr_inside_parens),
	test(test_allow_variable_shadowing_in_nested_blocks),
	test(test_allow_variable_shadowing_in_if_statements),
	test(test_allow_variable_shadowing_in_else_branch_if_statements),
	test(test_parse_recursive_function),
	test(test_parse_function_param_in_expr),
	test(test_register_unnamed_function_param),
	test(test_inner_struct_decl_is_anonymous),
	test(test_inner_enum_decl_is_anonymous),
	test(test_map_current_struct_fields),
	test(test_map_current_and_inner_anonymous_struct_fields),
	test(test_fields_not_mapped_for_struct_defined_inline_with_the_named_field),
	test(test_parse_union_def),
};

static TestCase compiler_tests[] = {
	test(test_reassigning_to_an_old_value_does_not_place_phi),
};

static TestCase x64_tests[] = {
	test(test_return_uint64_zero),
	test(test_add_uint64_consts),
	test(test_return_first_arg),
	test(test_return_sum_of_first_two_args),
	test(test_deref_function_arg),
	test(test_index_arary_with_pointer_arithmetics),
	test(test_index_arary_with_pointer_arithmetics_2),
	test(test_compare_equal_two_uint64),
	test(test_compare_equal_less_for_uint64),
	test(test_compare_equal_greater_for_uint64),
	test(test_mutate_argument),
	test(test_call_inside_inner_scope),
	test(test_conditional_call_1),
	test(test_conditional_call_2),
	test(test_conditional_call_between_two_calls_1),
	test(test_conditional_call_between_two_calls_2),
};

static TestSuite s_test_suites[] = {
	test_suite(source_info_tests),
	test_suite(tokenizer_tests),
	test_suite(preprocessor_tests),
	test_suite(parser_tests),
	test_suite(compiler_tests),
	test_suite(x64_tests),
};

bool arg_parse_uint64(const char* string, uint64_t* out) {
	errno = 0;
	char* end = NULL;
	uint64_t value = strtoull(string, &end, 10);

	if (end == string) {
		return false;
	}

	if (errno == ERANGE) {
		return false;
	}

	*out = value;
	return true;
}

bool test_cmd_parse(const char* string, TestCommandKind* kind) {
	uint64_t command_index = UINT64_MAX;
	if (!arg_parse_uint64(string, &command_index)) {
		return false;
	}

	if (command_index < (uint64_t)TEST_CMD_COUNT) {
		*kind = (TestCommandKind)command_index;
		return true;
	}

	return false;
}

void run_preprocessor_test(const char* file_path, Arena* arena, Arena* temp_arena) {
	String file_path_string = str_from_cstr(file_path);
	String source_code = read_entire_file_to_str(file_path, arena);

	SourceStorage source_storage = {};
	source_storage_init(&source_storage, (StringArray) {}, arena);
	SourceFile* source_file = source_storage_append(&source_storage, file_path_string, source_code);

	Diagnostics diagnostics = (Diagnostics) {
		.allocator = arena,
	};

	Arena generated_tokens_arena = arena_alloc_sub_arena(arena, 2 * 4096);
	
	Preprocessor preprocessor = {};
	preprocessor_init(&preprocessor,
			&source_storage,
			source_file,
			&diagnostics,
			arena,
			temp_arena,
			&generated_tokens_arena);

	Token first_token = {};
	bool has_token = false;

	while (true) {
		Token token = preprocessor_next_token(&preprocessor);
		if (token.kind == TOKEN_EOF) {
			break;
		} else {
			assert_msg(!has_token, "Preprocessor generated multiple tokens");
			first_token = token;
			has_token = true;
		}
	}

	assert_msg(has_token, "Preprocessor hasn't generated any tokens");
	assert(first_token.kind == TOKEN_IDENT);

	String pass_string = STR_LIT("pass");
	String fail_string = STR_LIT("fail");

	bool is_pass = str_equal(pass_string, first_token.string);
	bool is_fail = str_equal(fail_string, first_token.string);
	assert_msg(is_pass || is_fail, "Expected to get `pass` or `fail` token");

	assert_msg(is_pass, "Failed");
}

int main(int argc, char* argv[]) {
	srand(2153);

	if (argc > 1) {
		TestCommandKind cmd = TEST_CMD_COUNT;
		if (!test_cmd_parse(argv[1], &cmd)) {
			fprintf(stderr, "Invalid command %s", argv[1]);
			return EXIT_FAILURE;
		}

		switch (cmd) {
		case TEST_CMD_GET_TEST_SUITE_COUNT: {
			size_t test_suite_count = array_size(s_test_suites);
			printf("%zu\n", test_suite_count);
			break;
		}
		case TEST_CMD_GET_TEST_SUITE_NAMES: {
			size_t test_suite_count = array_size(s_test_suites);
			for (size_t i = 0; i < test_suite_count; i += 1) {
				printf("%.*s\n", STR_FMT(s_test_suites[i].name));
			}
			break;
		}
		case TEST_CMD_GET_TEST_COUNT: {
			size_t test_suite_count = array_size(s_test_suites);
			const char* input_promt = "args: <suite_index>";
			if (argc != 3) {
				fprintf(stderr, "%s", input_promt);
				return EXIT_FAILURE;
			}

			uint64_t suite_index = UINT64_MAX;

			if (!arg_parse_uint64(argv[2], &suite_index)) {
				fprintf(stderr, "Invalid <suite_index>");
				return EXIT_FAILURE;
			}

			if (suite_index >= test_suite_count) {
				fprintf(stderr, "Suite index out of range: [0; %zu]", test_suite_count - 1);
				return EXIT_FAILURE;
			}

			const TestSuite* suite = &s_test_suites[suite_index];
			printf("%zu", suite->case_count);
			return 0;
		}
		case TEST_CMD_GET_TEST_NAMES: {
			size_t test_suite_count = array_size(s_test_suites);
			const char* input_promt = "args: <suite_index>";
			if (argc != 3) {
				fprintf(stderr, "%s", input_promt);
				return EXIT_FAILURE;
			}

			uint64_t suite_index = UINT64_MAX;

			if (!arg_parse_uint64(argv[2], &suite_index)) {
				fprintf(stderr, "Invalid <suite_index>");
				return EXIT_FAILURE;
			}

			if (suite_index >= test_suite_count) {
				fprintf(stderr, "Suite index out of range: [0; %zu]", test_suite_count - 1);
				return EXIT_FAILURE;
			}

			const TestSuite* suite = &s_test_suites[suite_index];
			for (size_t i = 0; i < suite->case_count; i += 1) {
				printf("%.*s\n", STR_FMT(suite->cases[i].name));
			}
			return 0;
		}
		case TEST_CMD_RUN_TEST: {
			size_t test_suite_count = array_size(s_test_suites);
			const char* input_promt = "args: <suite_index> <test_index>";
			if (argc != 4) {
				fprintf(stderr, "%s", input_promt);
				return EXIT_FAILURE;
			}

			uint64_t suite_index = UINT64_MAX;
			uint64_t test_index = UINT64_MAX;

			if (!arg_parse_uint64(argv[2], &suite_index)) {
				fprintf(stderr, "Invalid <suite_index>");
				return EXIT_FAILURE;
			}

			if (suite_index >= test_suite_count) {
				fprintf(stderr, "Suite index out of range: [0; %zu]", test_suite_count - 1);
				return EXIT_FAILURE;
			}

			const TestSuite* suite = &s_test_suites[suite_index];

			if (!arg_parse_uint64(argv[3], &test_index)) {
				fprintf(stderr, "Invalid <test_index>");
				return EXIT_FAILURE;
			}

			if (test_index >= suite->case_count) {
				fprintf(stderr, "Test index out of range: [0; %zu]", suite->case_count - 1);
				return EXIT_FAILURE;
			}

			Arena arena = { .capacity = 128 * 4096 };
			Arena temp_arena = { .capacity = 128 * 4096 };

			const TestCase* test = &suite->cases[test_index];
			TestContext context = { .arena = &arena, .temp_arena = &temp_arena };
			test->function(&context);

			arena_release(&temp_arena);
			arena_release(&arena);
			return 0;
		}
		case TEST_CMD_RUN_PREPROCESSOR_TEST: {
			const char* input_promt = "args: <test_file_path>";
			
			if (argc != 3) {
				fprintf(stderr, "%s", input_promt);
				return EXIT_FAILURE;
			}

			const char* test_file_path = argv[2];

			Arena arena = { .capacity = 128 * 4096 };
			Arena temp_arena = { .capacity = 128 * 4096 };
			run_preprocessor_test(test_file_path, &arena, &temp_arena);

			arena_release(&temp_arena);
			arena_release(&arena);
			return 0;
		}
		case TEST_CMD_COUNT:
			unreachable();
		}
	}

	return 0;
}
