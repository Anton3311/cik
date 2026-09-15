#include <stdint.h>

__declspec(dllimport) void assert(uint64_t);

int main(int argc, char* argv[]) {
	int* initial = (int*)0xa;

	{
		int* a = initial;
		int* a1 = a++;
		assert(a1 == initial);
		assert(a == initial + 1);
	}

	{
		int* a = initial;
		int* a1 = ++a;
		assert(a1 == initial + 1);
		assert(a == initial + 1);
	}

	{
		int* a = initial;
		int* a1 = a--;
		assert(a1 == initial);
		assert(a == initial - 1);
	}

	{
		int* a = initial;
		int* a1 = --a;
		assert(a1 == initial - 1);
		assert(a == initial - 1);
	}
	return 0;
}
