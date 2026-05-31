#include <stdint.h>

void assert(uint64_t);

int main(int argc, char* argv[]) {
	// 8-bit unsigned
	assert(5ui8 - 10ui8 == 0xffui8 - 5ui8 + 1ui8);

	// 32-bit unsigned
	assert(5 - 10 == 0xffffffff - 5 + 1);

	// 64-bit unsigned
	assert(5 - 10ull == 0xffffffffffffffffull - 5 + 1);
	return 0;
}
