#include <stdint.h>

void assert(uint64_t);

int main(int argc, char* argv[]) {
	assert(5i8 | 3i8 == 7i8);
	assert(5i16 | 3i16 == 7i16);
	assert(5i32 | 3i32 == 7i32);
	assert(5i64 | 3i64 == 7i64);

	assert(5ui8 | 3ui8 == 7ui8);
	assert(5ui16 | 3ui16 == 7ui16);
	assert(5ui32 | 3ui32 == 7ui32);
	assert(5ui64 | 3ui64 == 7ui64);
	return 0;
}
