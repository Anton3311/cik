#include <stdint.h>

#include "lib.h"

__declspec(dllimport) void assert(uint64_t);

static uint64_t static_function() {
	return 10;
}

int main(int argc, char *argv[]) {
	assert(static_function() == 10);
	assert(call_other_static_function() == 50);
	return 0;
}
