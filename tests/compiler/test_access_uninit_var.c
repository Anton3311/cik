#include <stdint.h>

extern void assert(uint64_t);

int main(int argc, char* argv[]) {
	char a;
	assert(a == a);
	short b;
	assert(b == b);
	int c;
	assert(c == c);
	long long d;
	assert(d == d);

	return 0;
}
