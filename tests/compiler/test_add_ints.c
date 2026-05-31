#include <stdint.h>

void assert(uint64_t);

int main(int argc, char* argv[]) {
	assert(10 + 5 == 15);

	assert(8i8 + 2i8 == 10i8);
	return 0;
}
