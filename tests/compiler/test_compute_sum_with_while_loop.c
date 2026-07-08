#include <stdint.h>

void assert(uint64_t);

int main(int argc, char* argv[]) {
	int i = 0;
	int sum = 0;
	while (i < 20) {
		sum = sum + i;
		i = i + 1;
	}

	assert(sum == 190);
	return 0;
}
