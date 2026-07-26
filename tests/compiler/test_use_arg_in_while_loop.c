#include <stdint.h>

extern void assert(uint64_t);

int main(int argc, char* argv[]) {
	int iter_count = 0;
	int initial_argc = argc;
		
	while (argc > 0) {
		argc = argc - 1;
		iter_count = iter_count + 1;
	}

	assert(argc == 0);
	assert(initial_argc == iter_count);
	return 0;
}
