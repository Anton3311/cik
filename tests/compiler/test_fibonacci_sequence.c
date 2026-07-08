#include <stdint.h>

void assert(uint64_t);
int* malloc(size_t);
void free(int*);

int main(int argc, char* argv[]) {
	int i = 0;
	int a = 0;
	int b = 1;

	int* buffer = malloc(20);

	while (i < 5) {
		buffer[i] = a;

		int c = a + b;
		a = b;
		b = c;

		i = i + 1;
	}

	assert(buffer[0] == 0);
	assert(buffer[1] == 1);
	assert(buffer[2] == 1);
	assert(buffer[3] == 2);
	assert(buffer[4] == 3);

	free(buffer);
	return 0;
}
