#include <stdint.h>
#include <stdio.h>

__declspec(dllimport) void assert(unsigned long long);

#define unreachable() assert(0)

typedef uint8_t bool;
#define true ((bool)1)
#define false ((bool)0)

typedef struct {
	const char* v;
	size_t length;
} String;

#define STR_LIT(string) (String) { .v = string, .length = sizeof(string) - 1 }
#define STR_FMT(string) (int)(string).length, (string).v

inline bool str_equal(String str, String other) {
	if (str.length != other.length) {
		return false;
	}

	for (size_t i = 0; i < str.length; i += 1) {
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

		printf("%zu\n", str.length);
		printf("%d\n", (int)str.v[last_index]);
		printf("%d\n", (int)str.v[last_index - 1]);
		printf("%d\n", (int)(str.v[last_index - 1] == '\r' && str.v[last_index] == '\n'));

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
		printf("lf");
		return (String) { .v = str.v, .length = str.length - 1 };
	case LINE_ENDING_CRLF:
		printf("crlf");
		return (String) { .v = str.v, .length = str.length - 2 };
	}

	unreachable();
	return (String) {};
}

int main(int argc, char* argv[]) {
	String s0 = str_trim_line_ending(STR_LIT("hello world\r\n"));
	assert(str_equal(s0, STR_LIT("hello world")));

	String s1 = str_trim_line_ending(STR_LIT("hello world\n"));
	assert(str_equal(s1, STR_LIT("hello world")));

	String s2 = str_trim_line_ending(STR_LIT("hello world"));
	assert(str_equal(s2, STR_LIT("hello world")));
	return 0;
}
