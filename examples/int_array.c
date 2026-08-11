#include <stdint.h>
#include <stdio.h>

__declspec(dllimport) void* malloc(size_t);
__declspec(dllimport) void free(void*);
__declspec(dllimport) void assert(uint64_t);

#define NULL (void*)0

typedef struct {
	int* values;
	size_t count;
	size_t capacity;
} IntArray;

IntArray* int_array_alloc() {
	IntArray* array = (IntArray*)malloc(8 * 3);
	array->values = NULL;
	array->count = 0;
	array->capacity = 0;
	return array;
}

size_t max(size_t a, size_t b) {
	if (a > b) {
		return a;
	}
	return b;
}

void int_array_copy_values(int* src, int* dst, size_t count) {
	for (size_t i = 0; i < count; i += 1) {
		dst[i] = src[i];
	}
}

void int_array_grow(IntArray* array) {
	size_t new_capacity = max(array->capacity * 2, 4);

	int* new_values = malloc(new_capacity * 4);
	if (array->values) {
		int_array_copy_values(array->values, new_values, array->count);
		free(array->values);
	}

	array->values = new_values;
	array->capacity = new_capacity;
}

void int_array_append(IntArray* array, int value) {
	if (array->count == array->capacity) {
		int_array_grow(array);
	}

	array->values[array->count] = value;
	array->count += 1;
}

void int_array_remove_last(IntArray* array) {
	assert(array->count > 0);
	array->count -= 1;
}

void int_array_release(IntArray* array) {
	assert(array != NULL);

	if (array->values) {
		free(array->values);
	}

	free(array);
}

void int_array_print(const IntArray* array) {
	for (size_t i = 0; i < array->count; i += 1) {
		printf("%d ", array->values[i]);
	}
	printf("\n");
}

int main(int argc, char* argv[]) {
	IntArray* array = int_array_alloc();

	// Test initial grow
	int_array_append(array, 10);
	int_array_append(array, -100);
	int_array_append(array, 77);

	int_array_print(array);

	// Test remove
	int_array_remove_last(array);

	int_array_print(array);

	// Test grow
	int_array_append(array, 10);
	int_array_append(array, -100);
	int_array_append(array, 77);
	int_array_append(array, 88);
	int_array_append(array, 34);
	int_array_append(array, 17712);
	int_array_append(array, -9);

	int_array_print(array);

	int_array_release(array);

	return 0;
}
