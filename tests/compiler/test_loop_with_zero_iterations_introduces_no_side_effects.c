// Well, just test when a loop doesn't execute at all. It doesn't influence the state of the program
// in any way

#include <stdint.h>

__declspec(dllimport) void assert(uint64_t);
__declspec(dllimport) void panic(const char*);

int main(int argc, char* argv[]) {
	int a = 0;
	while (0) {
		a = 9;
		panic("Loop was not ment to execute");
	}

	assert(a == 0);
	return 0;
}
