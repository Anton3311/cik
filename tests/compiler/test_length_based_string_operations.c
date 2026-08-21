#include <stdio.h>
#include <stdint.h>

typedef uint8_t bool;
#define true (1)
#define false (0)

__declspec(dllimport) void assert(unsigned long long);
__declspec(dllimport) void* malloc(size_t);
__declspec(dllimport) void free(void*);

typedef struct {
	const char* v;
	size_t length;
} String;

#define STR_LIT(string) (String) { .v = string, .length = sizeof(string) - 1 }
#define STR_FMT(string) (int)(string).length, (string).v

bool str_equal(String a, String b) {
	if (a.length != b.length) {
		return false;
	}

	if (a.v == b.v) {
		return true;
	}

	for (size_t i = 0; i < a.length; i += 1) {
		if (a.v[i] != b.v[i]) {
			return false;
		}
	}

	return true;
}

char tolower(char c) {
	if (c >= 'A') {
		if (c <= 'Z') {
			return c - 'A' + 'a';
		}
	}

	return c;
}

String str_to_lower(String string) {
	char* lower = (char*)malloc(string.length);
	for (size_t i = 0; i < string.length; i += 1) {
		lower[i] = tolower(string.v[i]);
	}

	return (String) { .v = lower, .length = string.length };
}

void str_release(String string) {
	if (string.v) {
		free((void*)string.v);
	}
}

String sub_str(String str, size_t start, size_t length) {
	assert(start + length <= str.length);
	return (String) { .v = str.v + start, .length = length };
}

bool str_starts_with(String str, String prefix) {
	if (prefix.length > str.length) {
		return false;
	}

	return str_equal(sub_str(str, 0, prefix.length), prefix);
}

bool str_ends_with(String str, String sufix) {
	if (sufix.length > str.length) {
		return false;
	}

	return str_equal(sub_str(str, str.length - sufix.length, sufix.length), sufix);
}

int main(int argc, char *argv[]) {
	// str_equal
	assert(str_equal(STR_LIT("Hello world"), STR_LIT("Hello world")));
	assert(!str_equal(STR_LIT("Hello world"), STR_LIT("world")));
	assert(!str_equal(STR_LIT("Hello world"), STR_LIT("hello wolfs")));

	// str_to_lower
	String lowered = str_to_lower(STR_LIT("Hello WORld"));
	assert(str_equal(lowered, STR_LIT("hello world")));
	str_release(lowered);

	// sub_str
	assert(str_equal(sub_str(STR_LIT("Hello world"), 0, 5), STR_LIT("Hello")));

	// str_starts_with
	assert(str_starts_with(STR_LIT("Hello world"), STR_LIT("Hello")));
	assert(!str_starts_with(STR_LIT("Hello world"), STR_LIT("wood")));
	assert(str_starts_with(STR_LIT("Hello world"), STR_LIT("")));

	// str_ends_with
	assert(str_ends_with(STR_LIT("some text"), STR_LIT("text")));
	assert(!str_ends_with(STR_LIT("some text"), STR_LIT("end")));
	assert(str_ends_with(STR_LIT("some text"), STR_LIT("")));
	return 0;
}
