#include <stdio.h>

__declspec(dllimport) void assert(unsigned long long);

unsigned int factorial(unsigned int a) {
	if (a <= 1) {
		return 1;
	}

	return factorial(a - 1) * a;
}

unsigned int factorial_fast(unsigned int a) {
	unsigned int result = 1;
	for (unsigned int i = 1; i <= a; i += 1) {
		result *= i;
	}

	return result;
}

int main(int argc, char *argv[]) {
	for (unsigned int i = 0; i < 10; i += 1) {
		printf("%u\n", i);

		assert(factorial(i) == factorial_fast(i));
	}

	return 0;
}
