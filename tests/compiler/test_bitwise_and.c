#include <stdint.h>

void assert(uint64_t);

int main(int argc, char* argv[]) {
	assert((5i8 & 3i8) == 1i8);
	assert((5i16 & 3i16) == 1i16);
	assert((5i32 & 3i32) == 1i32);
	assert((5i64 & 3i64) == 1i64);

	assert((5ui8 & 3ui8) == 1ui8);
	assert((5ui16 & 3ui16) == 1ui16);
	assert((5ui32 & 3ui32) == 1ui32);
	assert((5ui64 & 3ui64) == 1ui64);
	return 0;
}
