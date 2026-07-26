#include <stdint.h>

extern void assert(uint64_t predicate);

int main(int argc, char* argv[]) {
	assert(20ui8 % 3ui8 == 2ui8);
	assert(20ui16 % 3ui16 == 2ui16);
	assert(20ui32 % 3ui32 == 2ui32);
	assert(20ui64 % 3ui64 == 2ui64);

	assert(17ui8 % 45ui8 == 17ui8);
	assert(17ui16 % 45ui16 == 17ui16);
	assert(17ui32 % 45ui32 == 17ui32);
	assert(17ui64 % 45ui64 == 17ui64);
	return 0;
}
