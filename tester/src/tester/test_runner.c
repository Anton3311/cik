#include <stdio.h>
#include <windows.h>

#include "core/core.h"
#include "tester/tester_core.h"

typedef struct {
	HANDLE read;
	HANDLE write;
} StdoutPipe;

bool stdout_pipe_create(StdoutPipe* pipe) {
	HANDLE stdout_read = NULL;
	HANDLE stdout_write = NULL;

	SECURITY_ATTRIBUTES security_attributes = {};
	security_attributes.nLength = sizeof(security_attributes);
	security_attributes.bInheritHandle = TRUE;
	security_attributes.lpSecurityDescriptor = NULL;
	if (!CreatePipe(&stdout_read, &stdout_write, &security_attributes, 0)) {
		return false;
	}

	*pipe = (StdoutPipe) {
		.read = stdout_read,
		.write = stdout_write,
	};

	return true;
}

void stdout_pipe_release(StdoutPipe* pipe) {
	CloseHandle(pipe->read);
	CloseHandle(pipe->write);
	*pipe = (StdoutPipe) {};
}

bool create_process_and_capture_stdout(const char* executable,
		const char* working_dir,
		char* args,
		Arena* allocator,
		String* out_stdout,
		int32_t* exit_code) {

	StdoutPipe pipe = {};
	if (!stdout_pipe_create(&pipe)) {
		return false;
	}

	PROCESS_INFORMATION process_info = {};
	STARTUPINFOA startup_info = {};

	ZeroMemory(&process_info, sizeof(process_info));
	ZeroMemory(&startup_info, sizeof(startup_info));

	startup_info.cb = sizeof(startup_info);
	startup_info.hStdError = pipe.write;
	startup_info.hStdOutput = pipe.write;
	startup_info.dwFlags |= STARTF_USESTDHANDLES;

	BOOL success = CreateProcessA(executable,
			args,
			NULL,
			NULL,
			TRUE,
			0,
			NULL,
			working_dir,
			&startup_info,
			&process_info);

	if (!success) {
		stdout_pipe_release(&pipe);
		return false;
	}

	WaitForSingleObject(process_info.hProcess, INFINITE);

	if (exit_code) {
		DWORD code = 0;
		GetExitCodeProcess(process_info.hProcess, &code);
		*exit_code = (int32_t)code;
	}

	CloseHandle(process_info.hProcess);
	CloseHandle(process_info.hThread);
	CloseHandle(pipe.write);

	String stdout_string = { .v = arena_alloc_array(allocator, char, 0), .length = 0 };
	size_t string_allocation_base = allocator->allocated;
	size_t buffer_size = 4096;

	while (true) {
		DWORD read_count = 0;

		char* buffer = arena_alloc_array(allocator, char, buffer_size);

		bool success = ReadFile(pipe.read, buffer, buffer_size, &read_count, NULL);
		if (!success || read_count == 0) {
			break;
		}

		stdout_string.length += (size_t)read_count;
	}

	allocator->allocated = string_allocation_base + stdout_string.length;
	*out_stdout = stdout_string;

	CloseHandle(pipe.read);
	return true;
}

bool run_tester(String args, Arena* arena, Arena* temp_arena, String* out_stdout, int32_t* exit_code) {
	ArenaRegion temp = arena_begin_temp(temp_arena);

	StringBuilder builder = { .arena = temp_arena };
	str_builder_append(&builder, STR_LIT("tester.exe "));
	str_builder_append(&builder, args);
	const char* command_line_args = str_builder_to_cstr(&builder);

	bool result = create_process_and_capture_stdout("bin/tester.exe",
			NULL,
			(char*)command_line_args,
			arena,
			out_stdout,
			exit_code);

	arena_end_temp(temp);

	return result;
}

typedef struct {
	String* suite_names;
	StringArray* suite_tests;
	size_t suite_count;
} TestStorage;

bool extract_test_suites(TestStorage* storage, Arena* arena, Arena* temp_arena) {
	{
		String all_test_suites = {};
		if (!run_tester(STR_LIT("1"), arena, temp_arena, &all_test_suites, NULL)) {
			return false;
		}

		StringArray test_suites = string_to_lines(all_test_suites, arena);
		storage->suite_names = test_suites.values;
		storage->suite_count = test_suites.count;
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

int main(int argc, char* argv[]) {
	Arena arena = { .capacity = 128 * 4096 };
	Arena temp_arena = { .capacity = 128 * 4096 };

	TestStorage test_storage = {};
	if (!extract_test_suites(&test_storage, &arena, &temp_arena)) {
		fprintf(stderr, "Failed to extract test cases\n");
		return EXIT_FAILURE;
	}

	for (size_t suite_index = 0; suite_index < test_storage.suite_count; suite_index += 1) {
		printf("%.*s:\n", STR_FMT(test_storage.suite_names[suite_index]));

		StringArray tests = test_storage.suite_tests[suite_index];
		for (size_t test_index = 0; test_index < tests.count; test_index += 1) {
			String test_name = tests.values[test_index];

			ArenaRegion temp = arena_begin_temp(&temp_arena);

			StringBuilder builder = { .arena = &temp_arena };
			str_builder_append_int(&builder, TEST_CMD_RUN_TEST);
			str_builder_append_char(&builder, ' ');
			str_builder_append_int(&builder, suite_index);
			str_builder_append_char(&builder, ' ');
			str_builder_append_int(&builder, test_index);

			String test_output = {};
			int32_t exit_code = 0;
			bool result = run_tester(builder.string, &arena, &temp_arena, &test_output, &exit_code);

			arena_end_temp(temp);

			bool pass = result && exit_code == 0;

			const char* status_string = pass ? "PASS" : "FAIL";

			const char* message = "";
			if (!result) {
				message = " - Failed to launch the test";
			}

			if (pass) {
				printf("\x1b[1;32m%s\x1b[0m %.*s %s\n", status_string, STR_FMT(test_name), message);
			} else {
				printf("\x1b[1;31m%s\x1b[0m %.*s %s\n", status_string, STR_FMT(test_name), message);
			}

			if (exit_code != 0) {
				printf("stdout:\n");
				printf("%.*s", STR_FMT(test_output));
				printf("exit code: %d\n", exit_code);
			}
		}
	}

	arena_release(&temp_arena);
	arena_release(&arena);
	return 0;
}
