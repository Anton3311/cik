#include <stdint.h>

__declspec(dllimport) void assert(uint64_t predicate);

int main(int argc, char* argv[]) {
	assert(2ui8 * 145ui8 == -222ui8);
	assert(0xffui32 * 0x0fffa123ui32 == 4020339165ui32);
	assert(0xffui64 * 0x0fffa123ffdecd01ui64 == 17267226327164793855ui64);
	return 0;
}
