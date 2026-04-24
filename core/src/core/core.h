#ifndef CORE_H
#define CORE_H

#include <stdalign.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef uint8_t bool;
typedef uint8_t bool8;
typedef uint32_t char32_t;

bool is_debugger_connected();
bool try_print_stack_trace(size_t skipped_frame_count);
void print_assertion_stack_trace();

#define true (bool)(1)
#define false (bool)(0)

#define debug_break() __debugbreak()
#define crash() if (is_debugger_connected()) { debug_break(); } else { exit(EXIT_FAILURE); }

#define has_flag(flag_set, flag) (((flag_set) & (flag)) == (flag))
#define has_any_flag(flag_set, flag) (((flag_set) & (flag)) != 0)

#define array_size(array) ((sizeof(array)) / sizeof(*(array)))
#define array_copy(dst, src, count) memcpy((dst), (src), sizeof(*(dst)) * (count))

#define assert_msg(expression, fmt, ...) if (!(expression)) { \
	print_assertion_stack_trace(); \
	printf("%s:%u: \033[31;1mAssertion '%s' failed\033[0m: ", \
			__FILE__, \
			__LINE__, \
	#expression); \
	printf(fmt, __VA_ARGS__); \
	crash(); }

#define assert(expression) if (!(expression)) { \
	print_assertion_stack_trace(); \
	printf("%s:%u: \033[31;1mAssertion '%s' failed\033[0m\n", \
			__FILE__, \
			__LINE__, \
	#expression); \
	crash(); }

#define unreachable() \
	print_assertion_stack_trace(); \
	printf("%s:%u: Reached unreachable statement.\n", __FILE__, __LINE__); \
	crash();

#define unreachable_msg(msg) \
	print_assertion_stack_trace(); \
	printf("%s:%u: %s\n", __FILE__, __LINE__, msg); \
	crash();

#define panic(msg) \
	print_assertion_stack_trace(); \
	printf("%s:%u: \033[31;1mPanic: '%s'\033[0m\n", __FILE__, __LINE__, msg); \
	crash();

#define debug_log_info(...) { printf("%s:%u: \033[32;1m", __FILE__, __LINE__); printf(__VA_ARGS__); printf("\033[0m\n"); }
#define debug_log_warn(...) { printf("%s:%u: \033[33;1m", __FILE__, __LINE__); printf(__VA_ARGS__); printf("\033[0m\n"); }
#define debug_log_error(...) { printf("%s:%u: \033[31;1m", __FILE__, __LINE__); printf(__VA_ARGS__); printf("\033[0m\n"); }

inline size_t align(size_t value, size_t alignment) {
	return (value + alignment - 1) / alignment * alignment;
}

uint64_t hardware_timer_get_frequency();
size_t align_to_page_size(size_t bytes);

//
// Bit Operations
//

#define count_leading_zeros(a) __builtin_clz(a)
#define count_trailing_zeros(a) __builtin_ctz(a)

//
// Allocator
//

typedef enum {
	ALLOC_OP_ALLOC,
	ALLOC_OP_FREE,
} AllocatorOperation;

typedef void*(*AllocatorProcedure)(void* allocator_data,
		void* ptr,
		size_t size,
		size_t alignment,
		AllocatorOperation op);

typedef struct {
	void* allocator_data;
	AllocatorProcedure procedure;
} Allocator;

inline void* allocator_alloc_bytes(Allocator allocator, size_t count, size_t alignment) {
	return allocator.procedure(allocator.allocator_data, NULL, count, alignment, ALLOC_OP_ALLOC);
}

inline void allocator_release(Allocator allocator, void* ptr) {
	assert(ptr != NULL);
	allocator.procedure(allocator.allocator_data, ptr, 0, 0, ALLOC_OP_FREE);
}

#define allocator_alloc(allocator, type) (type)allocator_alloc_bytes(allocator, sizeof(type), alignof(type));
#define allocator_alloc_array(allocator, type, count) \
	(type)allocator_alloc_bytes(allocator, sizeof(type) * count, alignof(type));

//
// Heap Allocator
//

inline void* heap_alloc_bytes(size_t count) {
	return malloc(count);
}

#define heap_alloc(type) (type*)heap_alloc_bytes(sizeof(type))
#define heap_alloc_array(type, count) (type*)heap_alloc_bytes(sizeof(type) * count)

inline void heap_release(void* ptr) {
	assert(ptr);
	free(ptr);
}

Allocator heap_allocator_new();

//
// Arena
//

typedef struct {
	size_t capacity;
	size_t commited;
	size_t allocated;
	uint8_t* base;
} Arena;

Allocator arena_allocator_new(Arena* arena);

void _arena_reserve(Arena* arena, size_t initial_size);
void _arena_commit(Arena* arena, size_t size);

inline void* arena_alloc_aligned(Arena* arena, size_t size, size_t alignment) {
	size_t allocation_base = align(arena->allocated, alignment);
	size_t new_allocated_ptr = allocation_base + size;

	if (arena->base == NULL) {
		_arena_reserve(arena, size);
	} else if (new_allocated_ptr > arena->commited) {
		_arena_commit(arena, new_allocated_ptr - arena->allocated);
	}

	void* allocation = arena->base + allocation_base;
	arena->allocated = new_allocated_ptr;
	return allocation;
}

inline void* arena_alloc_zeroed_aligned(Arena* arena, size_t size, size_t alignment) {
	void* ptr = arena_alloc_aligned(arena, size, alignment);
	memset(ptr, 0, size);
	return ptr;
}

inline Arena arena_alloc_sub_arena(Arena* arena, size_t size) {
	assert(arena != NULL);
	return (Arena) {
		.capacity = size,
		.commited = size,
		.allocated = 0,
		.base = arena_alloc_aligned(arena, size, 16),
	};
}

inline void arena_reset(Arena* arena) {
	arena->allocated = 0;
}

void arena_release(Arena* arena);

#define arena_alloc(arena, type) (type*)arena_alloc_aligned(arena, sizeof(type), alignof(type))
#define arena_alloc_zeroed(arena, type) (type*)arena_alloc_zeroed_aligned(arena, sizeof(type), alignof(type))
#define arena_alloc_array(arena, type, count) (type*)arena_alloc_aligned(arena, sizeof(type) * count, alignof(type))
#define arena_alloc_array_zeroed(arena, type, count) \
	(type*)arena_alloc_zeroed_aligned(arena, sizeof(type) * count, alignof(type))

//
// Temporary Arena allocations
//

typedef struct {
	Arena* arena;
	size_t allocated_state;
} ArenaRegion;

inline ArenaRegion arena_begin_temp(Arena* arena) {
	return (ArenaRegion) { .arena = arena, .allocated_state = arena->allocated };
}

inline void arena_end_temp(ArenaRegion save_point) {
	save_point.arena->allocated = save_point.allocated_state;
}

//
// Virtual Memory
//

void* allocate_executable(size_t size);
void free_executable(void* ptr, size_t size);

//
// Arrays
// 

typedef struct {
	uint16_t* values;
	size_t count;
} UInt16Array;

inline UInt16Array uint16_array_alloc(Arena* allocator, size_t count) {
	return (UInt16Array) {
		.values = arena_alloc_array(allocator, uint16_t, count),
		.count = count
	};
}

//
// String
//

inline bool is_digit(char c) {
	return '0' <= c && c <= '9';
}

typedef struct {
	const char* v;
	size_t length;
} String;

typedef struct {
	const wchar_t* v;
	size_t length;
} WideString;

typedef struct {
	String* values;
	size_t count;
} StringArray;

inline void str_array_append(StringArray* array, Arena* allocator, String value) {
	*arena_alloc(allocator, String) = value;
	array->count += 1;
}

#define STR_FMT(string) (int)(string).length, (string).v
#define STR_LIT(string) (String) { .v = string, .length = sizeof(string) - 1 }

inline String str_from_cstr(const char* cstr) {
	return (String) { .v = cstr, .length = strlen(cstr) };
}

inline String str_duplicate_from_cstr(const char* str, Arena* allocator) {
	size_t length = strlen(str);
	char* string = arena_alloc_array(allocator, char, length);
	memcpy(string, str, length);
	return (String) { .v = string, .length = length };
}

inline char* str_to_cstr(String string, Arena* allocator) {
	char* cstring = arena_alloc_array(allocator, char, string.length + 1);
	memcpy(cstring, string.v, string.length);
	cstring[string.length] = 0;
	return cstring;
}

WideString str_to_wstr(String string, Arena* allocator, bool include_null_terminator);
String str_from_wstr(WideString string, Arena* allocator);

inline String sub_str(String str, size_t start, size_t length) {
	assert(start + length <= str.length);
	return (String) { .v = str.v + start, .length = length };
}

#if 0
inline bool str_equal(String str, const char* cstr) {
	if (str.v == cstr) {
		size_t cstr_length = strlen(cstr);
		return cstr_length == str.length;
	}

	size_t i = 0;
	for (i = 0; i < str.length; i++) {
		if (cstr[i] != str.v[i]) {
			return false;
		}
	}

	// The both strings seem to be equal, so check whether ends of both strings were reached
	return cstr[i] == 0;
}
#endif

inline bool str_equal(String str, String other) {
	if (str.length != other.length) {
		return false;
	}

	for (size_t i = 0; i < str.length; i++) {
		if (str.v[i] != other.v[i]) {
			return false;
		}
	}

	return true;
}

typedef enum {
	LINE_ENDING_NONE,
	LINE_ENDING_LF,
	LINE_ENDING_CRLF,
} LineEndingType;

inline LineEndingType str_get_line_ending_type(String str) {
	if (str.length >= 2) {
		size_t last_index = str.length - 1;
		if (str.v[last_index - 1] == '\r' && str.v[last_index] == '\n') {
			return LINE_ENDING_CRLF;
		}
	}

	if (str.length >= 1) {
		size_t last_index = str.length - 1;
		if (str.v[last_index] == '\n') {
			return LINE_ENDING_LF;
		}
	}

	return LINE_ENDING_NONE;
}

inline String str_trim_line_ending(String str) {
	switch (str_get_line_ending_type(str)) {
	case LINE_ENDING_NONE:
		return str;
	case LINE_ENDING_LF:
		return (String) { .v = str.v, .length = str.length - 1 };
	case LINE_ENDING_CRLF:
		return (String) { .v = str.v, .length = str.length - 2 };
	}

	unreachable();
	return (String) {};
}

//
// String Builder
//

typedef struct {
	Arena* arena;
	String string;
} StringBuilder;

inline void str_builder_append(StringBuilder* builder, String string) {
	char* buffer = arena_alloc_array(builder->arena, char, string.length);
	memcpy(buffer, string.v, sizeof(char) * string.length);

	builder->string.length += string.length;

	if (!builder->string.v) {
		builder->string.v = buffer;
	}
}

inline void str_builder_append_cstr(StringBuilder* builder, const char* string) {
	str_builder_append(builder, str_from_cstr(string));
}

inline void str_builder_append_char(StringBuilder* builder, char c) {
	str_builder_append(builder, (String) { .v = &c, .length = 1 });
}

inline const char* str_builder_to_cstr(StringBuilder* builder) {
	char terminator = 0;
	str_builder_append(builder, (String) { .v = &terminator, .length = 1 });
	return builder->string.v;
}

void str_builder_append_int(StringBuilder* builder, uint64_t value);

//
// Line Iterator
//

typedef struct {
	String source;
	size_t read_position;
} LineIterator;

bool line_iterator_next(LineIterator* iter, String* out_line);
StringArray string_to_lines(String string, Arena* allocator);

//
// File System
//

typedef enum {
	FS_ENTRY_FILE = 1,
	FS_ENTRY_DIRECTORY = 2,
} FsEntryType;

String read_entire_file_to_str(const char* file_path, Arena* arena);

StringArray fs_enumerate_files_in_directory(String directory_path, Arena* file_path_allocator, Arena* temp_arena);
StringArray fs_enumerate_entries_in_directory(String directory_path,
		FsEntryType mask,
		Arena* file_path_allocator,
		Arena* temp_arena);

//
// Hashing
//

inline uint64_t murmur_hash_64(const void* key, size_t len, uint64_t seed)
{
    const uint64_t m = 0xc6a4a7935bd1e995ULL;
    const int r = 47;

    uint64_t h = seed ^ (len * m);

    const uint64_t* data = (const uint64_t *)key;
    const uint64_t* end = data + (len/8);

    while (data != end)
    {
        uint64_t k = *data++;

        k *= m;
        k ^= k >> r;
        k *= m;

        h ^= k;
        h *= m;
    }

    const unsigned char * data2 = (const unsigned char*)data;

    switch (len & 7)
    {
    case 7:
        h ^= (uint64_t)(data2[6]) << 48;
    case 6:
        h ^= (uint64_t)(data2[5]) << 40;
    case 5:
        h ^= (uint64_t)(data2[4]) << 32;
    case 4:
        h ^= (uint64_t)(data2[3]) << 24;
    case 3:
        h ^= (uint64_t)(data2[2]) << 16;
    case 2:
        h ^= (uint64_t)(data2[1]) << 8;
    case 1:
        h ^= (uint64_t)(data2[0]);
        h *= m;
    };

    h ^= h >> r;
    h *= m;
    h ^= h >> r;

    return h;
}

inline size_t hash_bytes(const void* bytes, size_t length) {
	return murmur_hash_64(bytes, length, 0x771238);
}

inline size_t hash_string(String string) {
	return hash_bytes(string.v, string.length);
}

inline size_t hash_ptr(const void* ptr) {
	return hash_bytes(&ptr, sizeof(ptr));
}

//
// Bit Array
//

typedef struct {
	uint8_t* values;
	size_t bit_count;
} BitArray;

inline BitArray bit_array_alloc(Arena* arena, size_t bit_count) {
	BitArray bit_array = {};
	static_assert(sizeof(*bit_array.values) == sizeof(uint8_t), "");

	size_t byte_count = (bit_count + 7) / 8;

	bit_array.values = arena_alloc_array(arena, uint8_t, byte_count);
	bit_array.bit_count = bit_count;

	return bit_array;
}

inline void bit_array_clear(BitArray* array) {
	memset(array->values, 0, (array->bit_count + 7) / 8);
}

inline bool bit_array_get(const BitArray* array, size_t index) {
	assert(index < array->bit_count);
	static_assert(sizeof(*array->values) == sizeof(uint8_t), "");

	size_t element_index = index / 8;
	size_t bit_index = index % 8;

	uint8_t mask = (uint8_t)(1 << bit_index);
	return (array->values[element_index] & mask) != 0;
}

inline void bit_array_set(const BitArray* array, size_t index, bool value) {
	assert(index < array->bit_count);
	static_assert(sizeof(*array->values) == sizeof(uint8_t), "");

	size_t element_index = index / 8;
	size_t bit_index = index % 8;

	uint8_t mask = (uint8_t)(1 << bit_index);
	if (value) {
		array->values[element_index] |= mask;
	} else {
		array->values[element_index] &= ~mask;
	}
}

inline void bit_array_and(const BitArray* a, const BitArray* b, BitArray* out) {
	assert(a->bit_count == b->bit_count && a->bit_count == out->bit_count);

	for (size_t i = 0; i < a->bit_count; i++) {
		out->values[i] = a->values[i] & b->values[i];
	}
}

inline void bit_array_or(const BitArray* a, const BitArray* b, BitArray* out) {
	assert(a->bit_count == b->bit_count && a->bit_count == out->bit_count);

	for (size_t i = 0; i < a->bit_count; i++) {
		out->values[i] = a->values[i] | b->values[i];
	}
}

//
// Registry
//

bool win_sdk_get_install_path(Arena* allocator, String* out_path);

//
// Path
//

String get_current_directory(Arena* allocator);
String path_to_absolute(Arena* allocator, String path);
bool path_exists(Arena* temp_allocator, String path);

bool path_is_file(Arena* temp_allocator, String path);
bool path_is_directory(Arena* temp_allocator, String path);

String path_canonicalize(String path, Arena* allocator, Arena* temp_allocator);
size_t path_get_file_name_start(String path);

// Returns 0 in case of error
uint64_t path_get_last_write_time(String path, Arena* temp_allocator);

inline String path_trim_trailing_slash(String path) {
	size_t trimmed_path_length = path.length;
	for (size_t i = path.length; i > 0; i -= 1) {
		char c = path.v[i - 1];
		if (c == '/' || c == '\\') {
			continue;
		} else {
			trimmed_path_length = i;
			break;
		}
	}

	return sub_str(path, 0, trimmed_path_length);
}

inline String path_get_file_name(String path) {
	size_t file_name_start = path_get_file_name_start(path);
	return sub_str(path, file_name_start, path.length - file_name_start);
}

inline String path_get_file_extension(String path) {
	String file_name = path_get_file_name(path);

	size_t dot_position = file_name.length;
	for (size_t i = 0; i < file_name.length; i += 1) {
		if (file_name.v[i] == '.') {
			dot_position = i;
		}
	}

	return sub_str(file_name, dot_position, file_name.length - dot_position);
}

inline String path_trim_file_extension(String path) {
	String file_name = path_get_file_name(path);

	size_t dot_position = file_name.length;
	for (size_t i = 0; i < file_name.length; i += 1) {
		if (file_name.v[i] == '.') {
			dot_position = i;
		}
	}

	return sub_str(file_name, 0, dot_position);
}

String path_get_parent(String path);
String path_append(String parent, String path, Arena* allocator);

//
// Process
//

typedef enum {
	PROCESS_RUN_OK,
	PROCESS_RUN_INVALID_EXE_PATH,
	PROCESS_RUN_INVALID_WORKING_DIR_PATH,
	PROCESS_RUN_ERROR,
} ProcessRunResult;

ProcessRunResult process_run(String executable_path,
		String working_directory,
		String arguments,
		int32_t* out_exit_code,
		Arena* temp_allocator);

ProcessRunResult process_capture_stdout(String executable_path,
		String working_directory,
		String arguments,
		int32_t* out_exit_code,
		String* out_stdout,
		Arena* allocator,
		Arena* temp_allocator);

#endif
