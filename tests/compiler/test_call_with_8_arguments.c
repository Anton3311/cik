#include <stdio.h>

__declspec(dllimport) void assert(unsigned long long);

int sum_8(int a, int b, int c, int d, int e, int f, int g, int h) {
	return a + b + c + d + e + f + g + h;
}

int main(int argc, char *argv[]) {
	assert(sum_8(0, 1, 2, 3, 4, 5, 6, 7) == (0 + 7) * 8 / 2);
	return 0;
}
