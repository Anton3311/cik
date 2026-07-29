#include <stdint.h>

__declspec(dllimport) void assert(uint64_t);
__declspec(dllimport) void panic(const char*);

int main(int argc, char* argv[]) {
	int initial_argc = argc;
	int i = 0;
	while (i > 0) {
		argc = argc - 1;
		panic("Loop was not ment to execute");
	}

	assert(initial_argc == argc);
	return 0;
}
