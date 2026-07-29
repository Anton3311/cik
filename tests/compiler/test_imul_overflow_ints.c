#include <stdint.h>

__declspec(dllimport) void assert(uint64_t predicate);

int main(int argc, char* argv[]) {
	assert(2i8 * 145i8 == -222i8);
	assert(0xffi32 * 0x0fffa123i32 == -274628131i32);
	assert(0xffi64 * 0x0fffa123ffdecd01i64 == -1179517746544757761i64);
	return 0;
}
