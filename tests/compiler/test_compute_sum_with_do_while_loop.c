#include <stdint.h>

__declspec(dllimport) void assert(uint64_t);
__declspec(dllimport) void printf(const char*, ...);

int main(int argc, char* argv[]) {
	int i = 0;
	int sum = 0;

	do {
		sum = sum + i;
		i = i + 1;
	} while (i < 20);

	printf("sum: %d\n", sum);
	assert(sum == 190);
	return 0;
}
