#include <stdint.h>

void panic(const char*);
void assert(uint64_t);

int main(int argc, char* argv[]) {
	int i = 0;
	do {
		break;

		panic("Unreachable code");
	} while (i < 20);

	assert(i == 0);
	return 0;
}
