#include <stdint.h>

void assert(uint64_t);

int main(int argc, char* argv[]) {
	int a = 0;
	a += 1;
	assert(a == 1);

	int b = 10;
	b -= 4;
	assert(b == 6);
	return 0;
}
