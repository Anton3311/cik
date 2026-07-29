#include <stdint.h>

typedef unsigned long long uint64_t;

__declspec(dllimport) void assert(uint64_t);
__declspec(dllimport) void printf(const char*, ...);

__declspec(dllimport) void* malloc(size_t count);
__declspec(dllimport) void free(void* ptr);

int main(int argc, char* argv[]) {
	const char* source = "hello    world some other word";

	int read_position = 0;
	int token_length = 0;

	while (1) {
		if (source[read_position + token_length] == 0) {
			break;
		}

		if (source[read_position + token_length] >= 'a') {
			if (source[read_position + token_length] <= 'z') {
				token_length = token_length + 1;
			}
		} else {
			if (token_length > 0) {
				printf("%\"%.*s\"\n", token_length, source + read_position);
			}

			if (source[read_position + token_length] == 0) {
				break;
			}

			read_position = read_position + token_length + 1;
			token_length = 0;
		}
	}

	return 0;
}
