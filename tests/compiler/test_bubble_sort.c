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

	int i = 0;
	while (i < 5) {
		int j = 0;
		while (j < 5) {
			if (array[j] < array[i]) {
				int temp = array[i];
				array[i] = array[j];
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
