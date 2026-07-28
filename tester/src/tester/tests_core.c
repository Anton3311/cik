#include "tests_core.h"

void test_alloc_array_using_empty_arena(TestContext* context) {
	Arena arena = { .capacity = 4096 };

	uint32_t* array = arena_alloc_array(&arena, uint32_t, 0);

	assert(array != NULL);
}
