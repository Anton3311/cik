#include <stdint.h>

void assert(uint64_t predicate);

int main(int argc, char* argv[]) {
	assert(20i8 / 8i8 == 2i8);
	assert(20i32 / 8i32 == 2i32);
	assert(20i64 / 8i64 == 2i64);

	assert(4i8 / 8i8 == 0i8);
	assert(4i32 / 8i32 == 0i32);
	assert(4i64 / 8i64 == 0i64);

	assert(-20i8 / 8i8 == -2i8);
	assert(-20i32 / 8i32 == -2i32);
	assert(-20i64 / 8i64 == -2i64);
	return 0;
}
