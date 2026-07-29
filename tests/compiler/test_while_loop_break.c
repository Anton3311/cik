#include <stdint.h>

__declspec(dllimport) void panic(const char*);
__declspec(dllimport) void assert(uint64_t);

int main(int argc, char* argv[]) {
	int i = 0;
	while (i < 20) {
		break;

		panic("Unreachable code");
	}

	assert(i == 0);
	return 0;
}
