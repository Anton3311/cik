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

int main(int argc, char* argv[]) {
	String ext0 = path_get_file_extension(STR_LIT("test.c"));
	assert(str_equal(ext0, STR_LIT(".c")));

	String ext1 = path_get_file_extension(STR_LIT("hello/test.c"));
	assert(str_equal(ext1, STR_LIT(".c")));

	String ext2 = path_get_file_extension(STR_LIT("hello\\hello/test.c"));
	assert(str_equal(ext2, STR_LIT(".c")));

	String ext3 = path_get_file_extension(STR_LIT("../..\\hello\\hello/test.c"));
	assert(str_equal(ext3, STR_LIT(".c")));

	String ext4 = path_get_file_extension(STR_LIT("../..\\hello\\hello/"));
	assert(str_equal(ext4, STR_LIT("")));

	String ext5 = path_get_file_extension(STR_LIT("../..\\hello\\hello"));
	assert(str_equal(ext5, STR_LIT("")));
	return 0;
}
