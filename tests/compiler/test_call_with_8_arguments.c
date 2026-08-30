#include <stdio.h>

__declspec(dllimport) void assert(unsigned long long);

int sum_8(int a, int b, int c, int d, int e, int f, int g, int h) {
	return a + b + c + d + e + f + g + h;
}

typedef struct {
	int a;
	int b;
	int c;
} Extra;

int sum_with_extra(int a, int b, int c, int d, int e, int f, int g, Extra extra) {
	return a + b + c + d + e + f + g + extra.a + extra.b + extra.c;
}

int return_last(int a, int b, int c, int d, int e, int f, int g, Extra extra) {
	return extra.c;
}

int main(int argc, char *argv[]) {
	assert(sum_8(0, 1, 2, 3, 4, 5, 6, 7) == (0 + 7) * 8 / 2);
	assert(sum_with_extra(0, 1, 2, 3, 4, 5, 6, (Extra) { 7, 8, 9 }) == (0 + 9) * 10 / 2);
	assert(return_last(0, 1, 2, 3, 4, 5, 6, (Extra) { 7, 8, 9 }) == 9);
	return 0;
}
