#include <stdint.h>

extern void* malloc(size_t count);
extern void free(void* ptr);
extern void panic(const char*);
extern void assert(uint64_t);

int main(int argc, char* argv[]) {
	int* array = malloc(20);
	array[0] = 89;
	array[1] = 34;
	array[2] = -12;
	array[3] = -9;
	array[4] = 7;

	int sum = 0;
	for (int i = 0; i < 5; i = i + 1) {
		if (array[i] < 0) {
			i = i + 1;
			continue;
		}

		sum = sum + array[i];
	}

	free(array);

	assert(sum == 130);
	return 0;
}

