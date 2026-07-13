#include <stdint.h>

void assert(uint64_t);

int main(int argc, char* argv[]) {
	int count = 0;

	int i = 0;
	do {
		int j = 0;
		do {
			j = j + 1;
			count = count + 1;
		} while (j < 5);

		i = i + 1;
	} while (i < 20);

	assert(count == 100);
	return 0;
}
