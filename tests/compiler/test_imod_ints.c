#include <stdint.h>

void assert(uint64_t predicate);

int main(int argc, char* argv[]) {
	assert(20i8 % 8i8 == 4i8);
	assert(20i16 % 8i16 == 4i16);
	assert(20i32 % 8i32 == 4i32);
	assert(20i64 % 8i64 == 4i64);

	assert(3i8 % 8i8 == 3i8);
	assert(3i16 % 8i16 == 3i16);
	assert(3i32 % 8i32 == 3i32);
	assert(3i64 % 8i64 == 3i64);

	assert(-20i8 % 7i8 == -6i8);
	assert(-20i16 % 7i16 == -6i16);
	assert(-20i32 % 7i32 == -6i32);
	assert(-20i64 % 7i64 == -6i64);
	return 0;
}
