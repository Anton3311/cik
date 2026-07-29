#include <stdint.h>

__declspec(dllimport) void assert(uint64_t predicate);

int main(int argc, char* argv[]) {
	assert(2ui8 * 4ui8 == 8ui8);
	assert(2ui16 * 4ui16 == 8ui16);
	assert(2ui32 * 4ui32 == 8ui32);
	assert(2ui64 * 4ui64 == 8ui64);
	return 0;
}
