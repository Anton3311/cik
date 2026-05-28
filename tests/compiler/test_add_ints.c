#include <stdint.h>

void assert(uint64_t);

int main(int argc, char* argv[]) {
	int a = 10 + 5;
	assert(a == 15);
	return 0;
}
