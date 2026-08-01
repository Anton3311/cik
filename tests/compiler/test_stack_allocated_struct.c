#include <stdint.h>

__declspec(dllimport) void assert(uint64_t);

typedef struct {
	int a;
	size_t b;
} MyStruct;

int main(int argc, char *argv[]) {
	MyStruct s;
	s.a = 10;
	s.b = 99;

	assert(s.a == 10);
	assert(s.b == 99);

	return 0;
}
