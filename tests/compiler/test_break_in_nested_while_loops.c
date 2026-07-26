#include <stdint.h>

extern void printf(const char*, ...);
extern void assert(uint64_t);

int main(int argc, char* argv[]) {
	int i = 0;
	int product = 0;
	while (1) {
		if (i == 10) {
			break;
		}

		int j = 0;
		while (1) {
			if (j == 5) {
				break;
			}

			product = product + 1;
			j = j + 1;
		}

		i = i + 1;
	}

	assert(product == 50);
	return 0;
}
