#include <stdint.h>

void assert(uint64_t);

int main(int argc, char* argv[]) {
	assert(10 - 5 == 5);
	return 0;
}
