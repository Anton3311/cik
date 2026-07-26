#include <stdint.h>

#define NULL ((void*)0)

void printf(const char*, ...);
void assert(uint64_t);

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

int main(int argc, char* argv[]) {
	assert(strcmp(NULL, NULL) == 0);

	assert(strcmp("hello", NULL) == 1);
	assert(strcmp(NULL, "hello") == -1);

	assert(strcmp("hello", "") == 1);
	assert(strcmp("", "hello") == -1);

	assert(strcmp("hello", "world") == -1);
	assert(strcmp("world", "hello") == 1);

	assert(strcmp("", "") == 0);
	assert(strcmp("hello", "hello") == 0);
	assert(strcmp("world", "world") == 0);

	assert(strcmp("hello", "hello   ") == -1);
	assert(strcmp("world", "world       ") == -1);

	assert(strcmp("hello  ", "hello") == 1);
	assert(strcmp("world     ", "world") == 1);
	return 0;
}
