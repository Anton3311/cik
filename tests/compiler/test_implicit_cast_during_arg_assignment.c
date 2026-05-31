#include <stdint.h>

void assert(uint64_t);

int main(int argc, char* argv[]) {
	argc = 0xffffffff00000010ull;
	assert(argc == 0x10);
	return 0;
}
