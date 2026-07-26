#include <stdint.h>

extern void assert(uint64_t);

int main(int argc, char* argv[]) {
	int i = 0;
	for (;;) {
		if (i == 10) {
			break;
		}

		i = i + 1;
	}

	assert(i == 10);
	return 0;
}
