#include <stdint.h>

extern void assert(uint64_t);
extern void printf(const char*, ...);

int main(int argc, char* argv[]) {
	int i = 0;
	int sum = 0;

	do {
		// Just split the addition into a sequence of +1
		int j = 0;
		while (j < i) {
			sum = sum + 1;
			j = j + 1;
		}

		i = i + 1;
	} while (i < 20);

	assert(sum == 190);
	return 0;
}
