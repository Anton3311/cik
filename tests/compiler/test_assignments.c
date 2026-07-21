#include <stdint.h>

void assert(uint64_t);

int main(int argc, char* argv[]) {
	int a = 0;
	a += 1;
	assert(a == 1);

	int b = 10;
	b -= 4;
	assert(b == 6);

	int c = 10;
	c *= 4;
	assert(c == 40);

	int d = 10;
	d /= 4;
	assert(d == 2);

	int e = 47;
	e %= 8;
	assert(e == 7);
	return 0;
}
