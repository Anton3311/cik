#include <stdint.h>

__declspec(dllimport) void assert(uint64_t);
__declspec(dllimport) int* malloc(size_t);
__declspec(dllimport) void free(void*);

int main(int argc, char* argv[]) {
	int* array = malloc(20);
	array[0] = 4;
	array[1] = -2;
	array[2] = 88;
	array[3] = 67;
	array[4] = -19;

	int i = 0;
	while (i < 5) {
		int j = 0;
		while (j < 4) {
			if (array[j + 1] < array[j]) {
				int temp = array[j + 1];
				array[j + 1] = array[j];
				array[j] = temp;
			}

			j = j + 1;
		}

		i = i + 1;
	}

	assert(array[0] = -19);
	assert(array[1] = -2);
	assert(array[2] = 4);
	assert(array[3] = 67);
	assert(array[4] = 88);

	free(array);
	return 0;
}
