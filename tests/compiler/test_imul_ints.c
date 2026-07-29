#include <stdint.h>

__declspec(dllimport) void assert(uint64_t predicate);

int main(int argc, char* argv[]) {
	assert(2i8 * 4i8 == 8i8);
	assert(2i16 * 4i16 == 8i16);
	assert(2i32 * 4i32 == 8i32);
	assert(2i64 * 4i64 == 8i64);
	return 0;
}
