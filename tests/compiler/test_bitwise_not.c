#include <stdint.h>

void assert(uint64_t);

int main(int argc, char* argv[]) {
	assert(~0i8 == 0xffi8);
	assert(~0i16 == 0xffffi16);
	assert(~0i32 == 0xffffffffi32);
	assert(~0i64 == 0xffffffffffffffffi64);

	assert(~0ui8 == 0xffui8);
	assert(~0ui16 == 0xffffui16);
	assert(~0ui32 == 0xffffffffui32);
	assert(~0ui64 == 0xffffffffffffffffui64);
	return 0;
}
