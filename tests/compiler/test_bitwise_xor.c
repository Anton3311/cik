#include <stdint.h>

__declspec(dllimport) void assert(uint64_t);

int main(int argc, char* argv[]) {
	assert(5i8 ^ 3i8 == 2i8);
	assert(5i16 ^ 3i16 == 2i16);
	assert(5i32 ^ 3i32 == 2i32);
	assert(5i64 ^ 3i64 == 2i64);

	assert(5ui8 ^ 3ui8 == 2ui8);
	assert(5ui16 ^ 3ui16 == 2ui16);
	assert(5ui32 ^ 3ui32 == 2ui32);
	assert(5ui64 ^ 3ui64 == 2ui64);
	return 0;
}
