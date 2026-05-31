#include <stdint.h>

void assert(uint64_t);

int main(int argc, char* argv[]) {
	uint8_t a = 256;
	assert(a == 0);
	return 0;
}
