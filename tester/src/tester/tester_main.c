#include <stdio.h>

#include "core/core.h"
#include "tester/tester_core.h"
#include "tester/tests.h"

static TestCase source_info_tests[] = {
	test(test_text_start_position_to_source_location),
	test(test_last_line_postion_to_source_location),
};

static TestCase tokenizer_tests[] = { 
	test(test_token_has_valid_string_represenation),
	test(test_token_source_range_matches_token_string),
	test(test_tokenizer_generates_expected_token),
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
};

static TestCase parser_tests[] = {
	test(test_parse_type_def_of_primitive_type),
	test(test_parse_type_def_of_struct_def),
	test(test_parse_type_def_of_struct_def_with_members),
	test(test_parse_enum_def),
	test(test_parse_function_def),
};

static TestSuite s_test_suites[] = {
	test_suite(source_info_tests),
	test_suite(tokenizer_tests),
	test_suite(preprocessor_tests),
	test_suite(parser_tests),
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
		case TEST_CMD_RUN_TEST:
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
		case TEST_CMD_COUNT:
			unreachable();
		}
	}

	return 0;
}
