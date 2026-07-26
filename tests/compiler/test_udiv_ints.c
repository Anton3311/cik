#include <stdint.h>

extern void assert(uint64_t);

int main(int argc, char* argv[]) {
	assert(20ui8 / 8ui8 == 2ui8);
	assert(20ui16 / 8ui16 == 2ui16);
	assert(20ui32 / 8ui32 == 2ui32);
	assert(20ui64 / 8ui64 == 2ui64);

	assert(4ui8 / 8ui8 == 0ui8);
	assert(4ui16 / 8ui16 == 0ui16);
	assert(4ui32 / 8ui32 == 0ui32);
	assert(4ui64 / 8ui64 == 0ui64);
	return 0;
}
