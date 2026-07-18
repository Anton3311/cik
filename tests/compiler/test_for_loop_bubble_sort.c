#include <stdint.h>

void assert(uint64_t);
int* malloc(size_t);
void free(void*);

int main(int argc, char* argv[]) {
	int* array = malloc(20);
	array[0] = 4;
	array[1] = -2;
	array[2] = 88;
	array[3] = 67;
	array[4] = -19;

	for (int i = 0; i < 5; i = i + 1) {
		for (int j = 0; j < 5; j = j + 1) {
			if (array[j + 1] < array[j]) {
				int temp = array[j + 1];
				array[j + 1] = array[j];
				array[j] = temp;
			}
		}
	}

	assert(array[0] = -19);
	assert(array[1] = -2);
	assert(array[2] = 4);
	assert(array[3] = 67);
	assert(array[4] = 88);

	free(array);
	return 0;
}
