#include <stdint.h>

__declspec(dllimport) void assert(uint64_t);

int main(int argc, char* argv[]) {
	int sum = 0;
	for (int i = 0; i < 20; i = i + 1) {
		sum = sum + i;
	}

	assert(sum == 190);
	return 0;
}
