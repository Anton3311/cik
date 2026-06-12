#include <stdint.h>

void assert(uint64_t);

int main(int argc, char* argv[]) {
	assert(-(-10i8) == 10i8);
#if 0
	assert(-(-10i16) == 10i16);
#endif
	assert(-(-10i32) == 10i32);
	assert(-(-10i64) == 10i64);
	return 0;
}
