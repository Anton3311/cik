#include "core.h"

#include <stdlib.h>
#include <stdio.h>

#include <windows.h>
#include <dbghelp.h>

bool is_debugger_connected() {
	return IsDebuggerPresent();
}

HANDLE _duplicate_process_handle(HANDLE process_handle)
{
	HANDLE process = NULL;
	if (!DuplicateHandle(process_handle, process_handle, process_handle, &process, 0, FALSE, DUPLICATE_SAME_ACCESS)) {
		return NULL;
	}

	return process;
}

bool try_print_stack_trace(size_t skipped_frame_count) {
	HANDLE process_handle = _duplicate_process_handle(GetCurrentProcess());
	if (process_handle == NULL) {
		return false;
	}

	SymSetOptions(SYMOPT_DEFERRED_LOADS | SYMOPT_UNDNAME);

	if (!SymInitialize(process_handle, NULL, TRUE)) {
		return false;
	}

	const size_t STACK_TRACE_BUFFER_SIZE = 64;
	void* stack_trace_buffer[STACK_TRACE_BUFFER_SIZE];

	const size_t MAX_NAME_LENGTH = 256;
	alignas(8) char symbol_buffer[sizeof(SYMBOL_INFO) + (MAX_NAME_LENGTH - 1) * sizeof(TCHAR)];

	size_t frame_count = CaptureStackBackTrace(
			skipped_frame_count,
			STACK_TRACE_BUFFER_SIZE,
			stack_trace_buffer,
			NULL);

	for (size_t i = 0; i < frame_count; i += 1) {
		size_t frame_index = frame_count - 1 - i;
		void* address = stack_trace_buffer[frame_index];

		PSYMBOL_INFO symbol_info = (PSYMBOL_INFO)symbol_buffer;
		symbol_info->SizeOfStruct = sizeof(SYMBOL_INFO);
		symbol_info->MaxNameLen = MAX_NAME_LENGTH;

		DWORD64 displacement = 0;
		if (SymFromAddr(process_handle, (DWORD64)address, &displacement, symbol_info)) {
			String symbol_name = (String) { symbol_info->Name, symbol_info->NameLen };
			printf("\t%zu: %p - %.*s\n", frame_index + 1, address, STR_FMT(symbol_name));
		} else {
			printf("\t%zu: %p\n", frame_index + 1, address);
		}
	}

	if (!SymCleanup(process_handle)) {
		// NOTE: SymCleanup failed, but we got the stack trace
	}

	return true;
}

void print_assertion_stack_trace() {
	bool result = try_print_stack_trace(2); // skip: `print_assertion_stack_trace` and `try_print_stack_trace`

	if (!result) {
		printf("\033[1;31mFailed to capture stack trace\033[0m");
	}
}

typedef struct {
	size_t page_size;
} SystemMemorysSpec;

static SystemMemorysSpec s_sys_mem_spec;

void _query_system_memory_spec() {
	if (s_sys_mem_spec.page_size != 0) {
		return;
	}

	SYSTEM_INFO sys_info = {};
	GetSystemInfo(&sys_info);

	s_sys_mem_spec.page_size = (size_t)sys_info.dwPageSize;
}

// NOTE: Assumes the sys mem spec had already been queried
inline size_t _compute_page_count(size_t bytes) {
	return (bytes + s_sys_mem_spec.page_size - 1) / s_sys_mem_spec.page_size;
}

size_t align_to_page_size(size_t bytes) {
	_query_system_memory_spec();
	return align(bytes, s_sys_mem_spec.page_size);
}

void _arena_reserve(Arena* arena, size_t initial_size) {
	assert(initial_size < arena->capacity);
	_query_system_memory_spec();

	size_t aligned_allocation = align(initial_size, s_sys_mem_spec.page_size);
	aligned_allocation = max(s_sys_mem_spec.page_size, aligned_allocation);

	arena->base = (uint8_t*)VirtualAlloc(NULL,
			(SIZE_T)align(arena->capacity, s_sys_mem_spec.page_size),
			MEM_RESERVE,
			PAGE_READWRITE);

	assert(arena->base != NULL);

	assert(VirtualAlloc(arena->base, (SIZE_T)aligned_allocation, MEM_COMMIT, PAGE_READWRITE) != NULL);

	arena->commited = aligned_allocation;
}

void _arena_commit_page(Arena* arena, size_t page_count) {
	size_t commit_size = page_count * s_sys_mem_spec.page_size;
	if (arena->commited + commit_size > arena->capacity) {
		printf("Out of arena memory");
		assert(false);
	}

	void* result = VirtualAlloc(arena->base + arena->commited,
			(SIZE_T)commit_size,
			MEM_COMMIT,
			PAGE_READWRITE);

	assert(result != NULL);

	arena->commited += commit_size;
}

void* arena_alloc_aligned(Arena* arena, size_t size, size_t alignment) {
	size_t allocation_base = align(arena->allocated, alignment);
	size_t new_allocated_ptr = allocation_base + size;

	if (arena->base == NULL) {
		_arena_reserve(arena, size);
	} else if (new_allocated_ptr > arena->commited) {
		_arena_commit_page(arena, _compute_page_count(new_allocated_ptr - arena->allocated));
	}

	void* allocation = arena->base + allocation_base;
	arena->allocated = new_allocated_ptr;
	return allocation;
}

void arena_release(Arena* arena) {
	if (arena->base == NULL) {
		return;
	}

	assert(VirtualFree(arena->base, 0, MEM_RELEASE) && "Failed to free arena");

	arena->base = NULL;
	arena->allocated = 0;
	arena->commited = 0;
}

void* allocate_executable(size_t size) {
	return VirtualAlloc(NULL, size, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
}

void free_executable(void* ptr, size_t size) {
	VirtualFree(ptr, size, MEM_RELEASE);
}

//
// String Builder
//

void str_builder_append_int(StringBuilder* builder, uint64_t value) {
	if (value == 0) {
		str_builder_append_char(builder, '0' + value);
		return;
	}

	char* buffer = arena_alloc_array(builder->arena, char, 0);

	if (!builder->string.v) {
		builder->string.v = buffer;
	}

	char* buffer_write_ptr = buffer;
	while (value) {
		uint32_t digit = value % 10;
		value /= 10;

		arena_alloc(builder->arena, char);

		*buffer_write_ptr = '0' + digit;
		buffer_write_ptr += 1;
	}

	size_t digit_count = buffer_write_ptr - buffer;

	for (size_t i = 0; i < digit_count / 2; i++) {
		char temp = buffer[digit_count - 1 - i];
		buffer[digit_count - 1 - i] = buffer[i];
		buffer[i] = temp;
	}

	builder->string.length += digit_count;
}

//
// Line Iterator
// 

bool line_iterator_next(LineIterator* iter, String* out_line) {
	if (iter->read_position >= iter->source.length) {
		return false;
	}

	size_t line_start = iter->read_position;
	size_t line_end = line_start;
	while (iter->read_position < iter->source.length) {
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

StringArray string_to_lines(String string, Arena* allocator) {
	StringArray array = { .values = arena_alloc_array(allocator, String, 0), .count = 0 };

	LineIterator iter = { .source = string };
	String line = {};
	while (line_iterator_next(&iter, &line)) {
		*arena_alloc(allocator, String) = line;
		array.count += 1;
	}

	return array;
}

//
// File IO
//

String read_entire_file_to_str(const char* file_path, Arena* arena) {
	FILE* f = NULL;
	errno_t error = fopen_s(&f, file_path, "rb");

	if (!f || error == EINVAL) {
		// Failed to read
		return (String) {};
	}

	fseek(f, 0, SEEK_END);
	size_t size = ftell(f);
	fseek(f, 0, SEEK_SET);

	char* string = arena_alloc_array(arena, char, size + 1);
	string[size] = 0;
	fread(string, 1, size, f);

	fclose(f);

	return (String) { .v = string, .length = size };
}

