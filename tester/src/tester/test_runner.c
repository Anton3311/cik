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
		String* out_stdout) {

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

typedef struct {
	String source;
	size_t read_position;
} LineIterator;

bool line_iterator_next(LineIterator* iter, String* out_line) {
	if (iter->read_position >= iter->source.length) {
		return false;
	}

	size_t line_start = iter->read_position;
	size_t line_end = line_start;
	while (iter->read_position != iter->source.length) {
		char current_char = iter->source.v[iter->read_position];
		line_end = iter->read_position;
		iter->read_position += 1;

		// skip \r\n
		if (current_char == '\r') {
			if (iter->read_position < iter->source.length && iter->source.v[iter->read_position] == '\n') {
				iter->read_position += 1;
			}

			break;
		}

		if (current_char == '\n') {
			break;
		}
	}

	String line = sub_str(iter->source, line_start, line_end - line_start);
	*out_line = line;
	return true;
}

int main(int argc, char* argv[]) {
	Arena arena = { .capacity = 128 * 4096 };
	char* tester_executable = "bin/tester.exe";

	String test_suites = {};
	if (!create_process_and_capture_stdout(tester_executable, NULL, "tester.exe 1", &arena, &test_suites)) {
		fprintf(stderr, "Failed to get test suites\n");
		return EXIT_FAILURE;
	}

	LineIterator line_iter = { .source = test_suites, .read_position = 0 };
	String line = {};
	while (line_iterator_next(&line_iter, &line)) {
		printf("%.*s\n", STR_FMT(line));
	}

	arena_release(&arena);
	return 0;
}
