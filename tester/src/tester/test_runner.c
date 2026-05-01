#include <stdio.h>
#include <windows.h>

#include "core/core.h"
#include "tester/tester_core.h"

bool run_tester(String args, Arena* arena, Arena* temp_arena, String* out_stdout, int32_t* out_exit_code) {
	ArenaRegion temp = arena_begin_temp(temp_arena);

	StringBuilder builder = { .arena = temp_arena };
	str_builder_append(&builder, STR_LIT("tester.exe "));
	str_builder_append(&builder, args);

	String cmd_args = builder.string;

	ProcessRunResult result = process_capture_stdout(STR_LIT("bin/tester.exe"),
			STR_LIT("."),
			cmd_args,
			out_exit_code,
			out_stdout,
			arena,
			temp_arena);

	arena_end_temp(temp);
	return result == PROCESS_RUN_OK;
}

typedef struct {
	String* suite_names;
	StringArray* suite_tests;
	size_t suite_count;
} TestStorage;

bool extract_test_suites(TestStorage* storage, Arena* arena, Arena* temp_arena) {
	{
		ArenaRegion temp = arena_begin_temp(temp_arena);
		String all_test_suites = {};

		StringBuilder builder = { .arena = temp_arena };
		str_builder_append_int(&builder, TEST_CMD_GET_TEST_SUITE_NAMES);
		
		if (run_tester(builder.string, arena, temp_arena, &all_test_suites, NULL)) {
			StringArray test_suites = string_to_lines(all_test_suites, arena);
			storage->suite_names = test_suites.values;
			storage->suite_count = test_suites.count;
		}

		arena_end_temp(temp);
	}

	storage->suite_tests = arena_alloc_array(arena, StringArray, storage->suite_count);

	for (size_t test_suite_index = 0; test_suite_index < storage->suite_count; test_suite_index += 1) {
		ArenaRegion temp = arena_begin_temp(temp_arena);

		StringBuilder builder = { .arena = temp_arena };
		str_builder_append_int(&builder, TEST_CMD_GET_TEST_NAMES);
		str_builder_append_char(&builder, ' ');
		str_builder_append_int(&builder, test_suite_index);

		String test_case_names = {};
		bool result = run_tester(builder.string, arena, temp_arena, &test_case_names, NULL);

		arena_end_temp(temp);

		if (!result) {
			return false;
		}

		storage->suite_tests[test_suite_index] = string_to_lines(test_case_names, arena);
	}

	return true;
}

bool run_single_test(String command, String test_name, Arena* arena, Arena* temp_arena) {
	ArenaRegion temp = arena_begin_temp(temp_arena);

	String test_output = {};
	int32_t exit_code = 0;
	bool result = run_tester(command, arena, temp_arena, &test_output, &exit_code);

	arena_end_temp(temp);

	bool pass = result && exit_code == 0;

	const char* status_string = pass ? "PASS" : "FAIL";

	const char* message = "";
	if (!result) {
		message = " - Failed to launch the test";
	}

	if (pass) {
		printf("  \x1b[1;32m%s\x1b[0m %.*s %s\n", status_string, STR_FMT(test_name), message);
	} else {
		printf("  \x1b[1;31m%s\x1b[0m %.*s %s\n", status_string, STR_FMT(test_name), message);
	}

	if (exit_code != 0) {
		printf("    stdout:\n");

		LineIterator iterator = (LineIterator) { .source = test_output };
		String output_line = {};
		while (line_iterator_next(&iterator, &output_line)) {
			printf("    | %.*s\n", STR_FMT(output_line));
		}

		printf("    exit code: %d\n", exit_code);
	}

	return pass;
}

int main(int argc, char* argv[]) {
	Arena arena = { .capacity = 128 * 4096 };
	Arena temp_arena = { .capacity = 128 * 4096 };

	TestStorage test_storage = {};
	if (!extract_test_suites(&test_storage, &arena, &temp_arena)) {
		fprintf(stderr, "Failed to extract test cases\n");
		return EXIT_FAILURE;
	}

	for (size_t suite_index = 0; suite_index < test_storage.suite_count; suite_index += 1) {
		printf("\n --- %.*s\n\n", STR_FMT(test_storage.suite_names[suite_index]));

		size_t tests_passed = 0;

		StringArray tests = test_storage.suite_tests[suite_index];
		for (size_t test_index = 0; test_index < tests.count; test_index += 1) {
			String test_name = tests.values[test_index];

			StringBuilder builder = { .arena = &temp_arena };
			str_builder_append_int(&builder, TEST_CMD_RUN_TEST);
			str_builder_append_char(&builder, ' ');
			str_builder_append_int(&builder, suite_index);
			str_builder_append_char(&builder, ' ');
			str_builder_append_int(&builder, test_index);

			if (run_single_test(builder.string, test_name, &arena, &temp_arena)) {
				tests_passed += 1;
			}
		}

		printf("\n  Passed: \033[1;32m%zu/%zu\033[0m Failed: \033[1;31m%zu/%zu\033[0m\n",
				tests_passed,
				tests.count,
				tests.count - tests_passed,
				tests.count);
	}

	{
		String test_directory = STR_LIT("tests/preprocessor");

		size_t tests_passed = 0;
		printf("\n --- %.*s\n\n", STR_FMT(test_directory));

		StringArray paths = fs_enumerate_files_in_directory(test_directory, &arena, &temp_arena);
		for (size_t i = 0; i < paths.count; i += 1) {
			StringBuilder builder = { .arena = &temp_arena };
			str_builder_append_int(&builder, TEST_CMD_RUN_PREPROCESSOR_TEST);
			str_builder_append_char(&builder, ' ');
			str_builder_append(&builder, test_directory);
			str_builder_append_char(&builder, '/');
			str_builder_append(&builder, paths.values[i]);

			if (run_single_test(builder.string, paths.values[i], &arena, &temp_arena)) {
				tests_passed += 1;
			}
		}

		size_t test_count = paths.count;
		printf("\n  Passed: \033[1;32m%zu/%zu\033[0m Failed: \033[1;31m%zu/%zu\033[0m\n",
				tests_passed,
				test_count,
				test_count - tests_passed,
				test_count);
	}

	arena_release(&temp_arena);
	arena_release(&arena);
	return 0;
}
