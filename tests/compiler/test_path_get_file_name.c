#include <stdint.h>
#include <stddef.h>

__declspec(dllimport) void assert(uint64_t);

typedef uint8_t bool;
#define true 1
#define false 0

int strcmp(const char* a, const char* b) {
	if (a == b) {
		return 0;
	}

	if (a == NULL) {
		return -1;
	}

	if (b == NULL) {
		return 1;
	}

	while (1) {
		if (*a < *b) {
			return -1;
		}

		if (*a > *b) {
			return 1;
		}

		if (*a == *b) {
			if (*a == 0) {
				break;
			}

			a += 1;
			b += 1;
		}
	}

	return 0;
}

typedef struct {
	const char* v;
	size_t length;
} String;

#define STR_FMT(string) (int)(string).length, (string).v
#define STR_LIT(string) (String) { .v = string, .length = sizeof(string) - 1 }

inline bool str_equal(String str, String other) {
	if (str.length != other.length) {
		return false;
	}

	return strcmp(str.v, other.v) == 0;
}

inline String sub_str(String str, size_t start, size_t length) {
	assert(start + length <= str.length);
	return (String) { .v = str.v + start, .length = length };
}

size_t path_get_file_name_start(String path) {
	for (size_t i = path.length; i > 0; i -= 1) {
		char c = path.v[i - 1];
		if (c == '/' || c == '\\') {
			return i;
		}
	}

	return 0;
}

inline String path_get_file_name(String path) {
	size_t file_name_start = path_get_file_name_start(path);
	return sub_str(path, file_name_start, path.length - file_name_start);
}

int main(int argc, char* argv[]) {
	String name0 = path_get_file_name(STR_LIT("test.c"));
	assert(str_equal(name0, STR_LIT("test.c")));

	String name1 = path_get_file_name(STR_LIT("hello/test.c"));
	assert(str_equal(name1, STR_LIT("test.c")));

	String name2 = path_get_file_name(STR_LIT("hello\\hello/test.c"));
	assert(str_equal(name2, STR_LIT("test.c")));

	String name3 = path_get_file_name(STR_LIT("../..\\hello\\hello/test.c"));
	assert(str_equal(name3, STR_LIT("test.c")));
	return 0;
}
