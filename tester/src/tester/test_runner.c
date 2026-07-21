#include <stdio.h>

#include "core/core.h"
#include "tester/tester_core.h"

#define PREPROCESSOR_TESTS_DIRECTORY "tests/preprocessor"
#define COMPILER_TESTS_DIRECTORY "tests/compiler"
#define TESTER_EXE_NAME "tester.exe"
#define TESTER_EXE_PATH "bin/tester.exe"

typedef struct {
	size_t suite_index;
	size_t test_index;
	String test_suite_name;
	String test_name;
	Arena* allocator;
	Arena* temp_allocator;
} TestRunnerContext;

typedef struct {
	// Captured stdout
	String output;
	ProcessRunResult process_run_result;
	int32_t exit_code;
} TestResult;

typedef TestResult (*TestRunner)(TestRunnerContext* context);

typedef struct {
	String name;
	TestRunner runner;
} TestDescriptor;

typedef struct {
	TestDescriptor* tests;
	size_t count;
} TestDescriptorArray;

typedef struct {
	String name;
	TestDescriptorArray tests;
} TestSuiteDescriptor;

typedef struct {
	TestSuiteDescriptor* suites;
	size_t suite_count;
} TestStorage;

typedef struct {
	size_t passed_count;
} Summary;

//
// Test Runners
// 

TestResult test_run_unit_test(TestRunnerContext* context) {
	StringBuilder builder = { .arena = context->temp_allocator };
	str_builder_append(&builder, STR_LIT(TESTER_EXE_NAME));
	str_builder_append_char(&builder, ' ');
	str_builder_append_int(&builder, TEST_CMD_RUN_TEST);
	str_builder_append_char(&builder, ' ');
	str_builder_append_int(&builder, context->suite_index);
	str_builder_append_char(&builder, ' ');
	str_builder_append_int(&builder, context->test_index);

	String test_output = {};
	int32_t exit_code = 0;
	ProcessRunResult process_result = process_capture_stdout(
			STR_LIT(TESTER_EXE_PATH),
			STR_LIT("."),
			builder.string,
			&exit_code,
			&test_output,
			context->allocator,
			context->temp_allocator);

	return (TestResult) {
		.output = test_output,
		.process_run_result = process_result,
		.exit_code = exit_code,
	};
}

TestResult test_run_preprocessor_test(TestRunnerContext* context) {
	StringBuilder builder = { .arena = context->temp_allocator };
	str_builder_append(&builder, STR_LIT(TESTER_EXE_NAME));
	str_builder_append_char(&builder, ' ');
	str_builder_append_int(&builder, TEST_CMD_RUN_PREPROCESSOR_TEST);
	str_builder_append_char(&builder, ' ');
	str_builder_append(&builder, STR_LIT(PREPROCESSOR_TESTS_DIRECTORY));
	str_builder_append_char(&builder, '/');
	str_builder_append(&builder, context->test_name);

	String test_output = {};
	int32_t exit_code = 0;
	ProcessRunResult process_result = process_capture_stdout(
			STR_LIT(TESTER_EXE_PATH),
			STR_LIT("."),
			builder.string,
			&exit_code,
			&test_output,
			context->allocator,
			context->temp_allocator);

	return (TestResult) {
		.output = test_output,
		.process_run_result = process_result,
		.exit_code = exit_code,
	};
}

TestResult test_run_compiler_test(TestRunnerContext* context) {
	String compiler_path = STR_LIT("bin/c.exe");

	StringBuilder builder = { .arena = context->temp_allocator };
	str_builder_append(&builder, compiler_path);
	str_builder_append_char(&builder, ' ');
	str_builder_append(&builder, STR_LIT(COMPILER_TESTS_DIRECTORY));
	str_builder_append_char(&builder, '/');
	str_builder_append(&builder, context->test_name);
	str_builder_append(&builder, STR_LIT(" --show-ir -Istdx"));

	String cmd_args = builder.string;

	String output;
	int32_t exit_code;
	ProcessRunResult process_result = process_capture_stdout(
			compiler_path,
			STR_LIT("."),
			cmd_args,
			&exit_code,
			&output,
			context->allocator,
			context->temp_allocator);

	return (TestResult) {
		.output = output,
		.process_run_result = process_result,
		.exit_code = exit_code,
	};
}

//
// Test Extraction
// 

bool extract_test_suites(TestStorage* storage, Arena* suites_allocator, Arena* tests_allocator) {
	storage->suites = arena_alloc_array(suites_allocator, TestSuiteDescriptor, 0);
	storage->suite_count = 0;

	{
		ArenaRegion temp = arena_begin_temp(suites_allocator);

		StringBuilder builder = { .arena = suites_allocator };
		str_builder_append(&builder, STR_LIT(TESTER_EXE_NAME));
		str_builder_append_char(&builder, ' ');
		str_builder_append_int(&builder, TEST_CMD_GET_TEST_SUITE_NAMES);

		String all_test_suites = {};
		int32_t exit_code = 0;
		bool result = process_capture_stdout(
				STR_LIT(TESTER_EXE_PATH),
				STR_LIT("."),
				builder.string,
				&exit_code,
				&all_test_suites,
				tests_allocator,
				suites_allocator) == PROCESS_RUN_OK;

		arena_end_temp(temp);

		if (result) {
			LineIterator iter = { .source = all_test_suites };
			String line = {};
			while (line_iterator_next(&iter, &line)) {
				TestSuiteDescriptor* desc = arena_alloc(suites_allocator, TestSuiteDescriptor);
				storage->suite_count += 1;

				desc->name = line;
				desc->tests = (TestDescriptorArray) {};
			}
		} else {
			return false;
		}
	}

	for (size_t test_suite_index = 0; test_suite_index < storage->suite_count; test_suite_index += 1) {
		ArenaRegion temp = arena_begin_temp(suites_allocator);

		StringBuilder builder = { .arena = suites_allocator };
		str_builder_append(&builder, STR_LIT(TESTER_EXE_NAME));
		str_builder_append_char(&builder, ' ');
		str_builder_append_int(&builder, TEST_CMD_GET_TEST_NAMES);
		str_builder_append_char(&builder, ' ');
		str_builder_append_int(&builder, test_suite_index);

		String test_case_names = {};
		int32_t exit_code = 0;
		bool result = process_capture_stdout(
				STR_LIT(TESTER_EXE_PATH),
				STR_LIT("."),
				builder.string,
				&exit_code,
				&test_case_names,
				tests_allocator,
				suites_allocator) == PROCESS_RUN_OK;

		arena_end_temp(temp);

		if (!result) {
			return false;
		}

		TestDescriptorArray* tests = &storage->suites[test_suite_index].tests;
		tests->tests = arena_alloc_array(tests_allocator, TestDescriptor, 0);

		LineIterator iter = { .source = test_case_names };
		String line = {};
		while (line_iterator_next(&iter, &line)) {
			TestDescriptor* desc = arena_alloc(tests_allocator, TestDescriptor);
			desc->name = line;
			desc->runner = test_run_unit_test;
			tests->count += 1;
		}
	}

	// Extract preprocessor tests

	{
		StringArray paths = fs_enumerate_files_in_directory(
				STR_LIT(PREPROCESSOR_TESTS_DIRECTORY),
				tests_allocator,
				suites_allocator);

		TestSuiteDescriptor* suite = arena_alloc(suites_allocator, TestSuiteDescriptor);
		suite->name = STR_LIT(PREPROCESSOR_TESTS_DIRECTORY);
		suite->tests.tests = arena_alloc_array(tests_allocator, TestDescriptor, paths.count);
		suite->tests.count = paths.count;

		storage->suite_count += 1;

		for (size_t i = 0; i < paths.count; i += 1) {
			TestDescriptor* test = &suite->tests.tests[i];
			test->name = paths.values[i];
			test->runner = test_run_preprocessor_test;

		}
	}

	// Extract compiler tests
	{
		StringArray paths = fs_enumerate_files_in_directory(
				STR_LIT(COMPILER_TESTS_DIRECTORY),
				tests_allocator,
				suites_allocator);

		TestSuiteDescriptor* suite = arena_alloc(suites_allocator, TestSuiteDescriptor);
		suite->name = STR_LIT(COMPILER_TESTS_DIRECTORY);
		suite->tests.tests = arena_alloc_array(tests_allocator, TestDescriptor, paths.count);
		suite->tests.count = paths.count;

		storage->suite_count += 1;

		for (size_t i = 0; i < paths.count; i += 1) {
			TestDescriptor* test = &suite->tests.tests[i];
			test->name = paths.values[i];
			test->runner = test_run_compiler_test;

		}
	}

	return true;
}

void report_termination_status(String output, int32_t exit_code) {
	printf("    stdout:\n");

	LineIterator iterator = (LineIterator) { .source = output };
	String output_line = {};
	while (line_iterator_next(&iterator, &output_line)) {
		printf("    | %.*s\n", STR_FMT(output_line));
	}

	printf("    exit code: %d\n", exit_code);
}

inline bool test_passed(ProcessRunResult process_result, int32_t exit_code) {
	return process_result == PROCESS_RUN_OK && exit_code == 0;
}

void report_test_result(String test_name,
		ProcessRunResult process_result,
		int32_t exit_code,
		String stdout_output) {

	bool pass = test_passed(process_result, exit_code);
	const char* status_string = pass ? "PASS" : "FAIL";

	const char* message = "";
	if (process_result != PROCESS_RUN_OK) {
		message = " - Failed to launch the test";
	}

	if (pass) {
		printf("  \x1b[1;32m%s\x1b[0m %.*s %s\n", status_string, STR_FMT(test_name), message);
	} else {
		printf("  \x1b[1;31m%s\x1b[0m %.*s %s\n", status_string, STR_FMT(test_name), message);
	}

	if (exit_code != 0) {
		report_termination_status(stdout_output, exit_code);
	}
}

// NOTE: Allocations from the `allocator` and `temp_allocator` are all cleaned up before the
//       function returns. Both allocator in this case act as temporary.
void run_test_and_report_result(TestStorage* storage,
		size_t suite_index,
		size_t test_index,
		Summary* summary,
		Arena* allocator,
		Arena* temp_allocator) {
	assert(suite_index < storage->suite_count);
	TestSuiteDescriptor* suite = &storage->suites[suite_index];

	assert(test_index < suite->tests.count);
	TestDescriptor* test = &suite->tests.tests[test_index];

	assert(test->runner);

	ArenaRegion temp1 = arena_begin_temp(allocator);
	ArenaRegion temp2 = arena_begin_temp(temp_allocator);

	TestRunnerContext context;
	context.suite_index = suite_index;
	context.test_index = test_index;
	context.test_suite_name = suite->name;
	context.test_name = test->name;
	context.allocator = allocator;
	context.temp_allocator = temp_allocator;

	TestResult result = test->runner(&context);

	// Report results
	bool pass = test_passed(result.process_run_result, result.exit_code);
	const char* status_string = pass ? "PASS" : "FAIL";

	const char* message = "";
	if (result.process_run_result != PROCESS_RUN_OK) {
		message = " - Failed to launch the test";
	}

	if (pass) {
		printf("  \x1b[1;32m%s\x1b[0m %.*s %s\n", status_string, STR_FMT(test->name), message);
	} else {
		printf("  \x1b[1;31m%s\x1b[0m %.*s %s\n", status_string, STR_FMT(test->name), message);
	}

	if (result.exit_code != 0) {
		printf("    stdout:\n");

		LineIterator iterator = (LineIterator) { .source = result.output };
		String output_line = {};
		while (line_iterator_next(&iterator, &output_line)) {
			printf("    | %.*s\n", STR_FMT(output_line));
		}

		printf("    exit code: %d\n", result.exit_code);
	}

	if (pass) {
		summary->passed_count += 1;
	}

	arena_end_temp(temp1);
	arena_end_temp(temp2);
}

typedef enum {
	MODE_ALL,
	MODE_SINGLE,
} ModeKind;

typedef struct {
	ModeKind kind;
	union {
		struct {
			size_t suite_index;
			size_t test_index;
		} single;
	};
} Mode;

int main(int argc, char* argv[]) {
	Arena arena = { .capacity = 128 * 4096 };
	Arena temp_arena = { .capacity = 128 * 4096 };

	TestStorage test_storage = {};
	if (!extract_test_suites(&test_storage, &arena, &temp_arena)) {
		fprintf(stderr, "Failed to extract test cases\n");
		return EXIT_FAILURE;
	}

	Mode mode = {};
	mode.kind = MODE_ALL;

	int32_t arg_index = 1;
	while (arg_index < argc) {
		if (strcmp(argv[arg_index], "run-test") == 0) {
			mode.kind = MODE_SINGLE;

			arg_index += 1;
			if (arg_index >= argc) {
				fprintf(stderr, "Expected test case name");
				return EXIT_FAILURE;
			}

			bool test_case_found = false;
			String test_case_name = str_from_cstr(argv[arg_index]);
			arg_index += 1;

			for (size_t suite_index = 0; suite_index < test_storage.suite_count; suite_index += 1) {
				const TestSuiteDescriptor* suite = &test_storage.suites[suite_index];
				for (size_t test_index = 0; test_index < suite->tests.count; test_index += 1) {
					if (str_equal(suite->tests.tests[test_index].name, test_case_name)) {
						mode.single.suite_index = suite_index;
						mode.single.test_index = test_index;
						test_case_found = true;
						break;
					}
				}

				if (test_case_found) {
					break;
				}
			}

			if (!test_case_found) {
				fprintf(stderr,
						"Test named '%.*s' not found in any of the test suites",
						STR_FMT(test_case_name));
				return EXIT_FAILURE;
			}
		} else {
			fprintf(stderr, "Unknown option '%s'", argv[arg_index]);
			return EXIT_FAILURE;
		}
	}

	bool has_failed_tests = false;

	switch (mode.kind) {
	case MODE_ALL:
		for (size_t suite_index = 0; suite_index < test_storage.suite_count; suite_index += 1) {
			printf("\n --- %.*s\n\n", STR_FMT(test_storage.suites[suite_index].name));

			Summary summary = {};

			TestDescriptorArray tests = test_storage.suites[suite_index].tests;
			for (size_t test_index = 0; test_index < tests.count; test_index += 1) {
				run_test_and_report_result(&test_storage,
						suite_index,
						test_index,
						&summary,
						&arena,
						&temp_arena);
			}

			printf("\n  Passed: \033[1;32m%zu/%zu\033[0m Failed: \033[1;31m%zu/%zu\033[0m Skipped: %zu/%zu\n",
					summary.passed_count,
					tests.count,
					tests.count - summary.passed_count,
					tests.count,
					(size_t)0,
					tests.count);

			if (summary.passed_count < tests.count) {
				has_failed_tests = true;
			}
		}
		break;
	case MODE_SINGLE: {
		Summary summary = {};

		run_test_and_report_result(&test_storage,
				mode.single.suite_index,
				mode.single.test_index,
				&summary,
				&arena,
				&temp_arena);

		size_t test_count = 1;
		printf("\n  Passed: \033[1;32m%zu/%zu\033[0m Failed: \033[1;31m%zu/%zu\033[0m Skipped: %zu\n",
				summary.passed_count,
				test_count,
				test_count - summary.passed_count,
				test_count,
				test_storage.suites[mode.single.suite_index].tests.count - test_count);

		if (summary.passed_count < test_count) {
			has_failed_tests = true;
		}
		break;
	}
	}


	arena_release(&temp_arena);
	arena_release(&arena);
	return has_failed_tests ? EXIT_FAILURE : 0;
}
