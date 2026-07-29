#include <stdint.h>

#include "func.h"

__declspec(dllimport) void assert(uint64_t);

int main(int argc, char* argv[]) {
	assert(add(10, 50) == 60);
	return 0;
}
