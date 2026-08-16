#include <stdio.h>

#define SIZE_MAX 0xffffffffffffffff

#define NULL (void*)0
#define array_size(array) (sizeof(array) / sizeof(array[0]))

__declspec(dllimport) void assert(unsigned long long);

void print_array(int* array, size_t count);

size_t move_left_pointer(int* array, size_t left, int pivot) {
	while (array[left] < pivot) {
		left += 1;
	}

	return left;
}

size_t move_right_pointer(int* array, size_t right, int pivot) {
	while (array[right] > pivot) {
		right -= 1;
	}

	return right;
}

size_t partition(int* array, size_t left, size_t right) {
	assert(left < right);

	int pivot = array[(left + right) / 2];

	while (1) {
		left = move_left_pointer(array, left, pivot);
		right = move_right_pointer(array, right, pivot);

		if (left >= right) {
			return right;
		}

		int temp = array[left];
		array[left] = array[right];
		array[right] = temp;
	}

	return SIZE_MAX;
}

void quick_sort_rec(int* array, size_t left, size_t right) {
	if (left >= right) {
		return;
	}

	size_t mid_point = partition(array, left, right);

	quick_sort_rec(array, left, mid_point - 1);
	quick_sort_rec(array, mid_point + 1, right);
}

void quick_sort(int* array, size_t count) {
	if (count >= 1) {
		quick_sort_rec(array, 0, count - 1);
	}
}

int verify(int* array, size_t count) {
	for (size_t i = 1; i < count; i += 1) {
		if (array[i - 1] > array[i]) {
			return 0;
		}	
	}

	return 1;
}

void print_array(int* array, size_t count) {
	for (size_t i = 0; i < count; i += 1) {
		printf("%d ", array[i]);
	}
	printf("\n");
}

int main(int argc, char *argv[]) {
	int array[10];
	array[0] = 760;
	array[1] = 214;
	array[2] = 578;
	array[3] = 378;
	array[4] = 401;
	array[5] = -162;
	array[6] = -368;
	array[7] = -662;
	array[8] = -705;
	array[9] = 450;

	printf("before: ");
	print_array(array, array_size(array));

	quick_sort(array, array_size(array));

	printf(" after: ");
	print_array(array, array_size(array));

	assert(verify(array, array_size(array)));
	return 0;
}
