#include <stdint.h>

void assert(uint64_t);

int main(int argc, char* argv[]) {
	assert(10 + 5 == 15);
	return 0;
}
