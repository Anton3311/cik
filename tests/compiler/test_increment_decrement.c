#include <stdint.h>

void assert(uint64_t);

int main(int argc, char* argv[]) {
	{
		int a = 0;
		int a1 = a++;
		assert(a1 == 0);
		assert(a == 1);
	}

	{
		int a = 0;
		int a1 = ++a;
		assert(a1 == 1);
		assert(a == 1);
	}

	{
		int a = 55;
		int a1 = a--;
		assert(a1 == 55);
		assert(a == 54);
	}

	{
		int a = 55;
		int a1 = --a;
		assert(a1 == 54);
		assert(a == 54);
	}

	return 0;
}
