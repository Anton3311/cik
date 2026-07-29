#include <stdint.h>

__declspec(dllimport) void assert(uint64_t);
__declspec(dllimport) void panic(const char*);

int main(int argc, char* argv[]) {
	assert(!(0i8 == 0i8) == 0);
	assert(!(0i16 == 0i16) == 0);
	assert(!(0i32 == 0i32) == 0);
	assert(!(0i64 == 0i64) == 0);

	assert(!!0 == 0);

	assert(!0 == 1);
	assert(!1 == 0);

	assert(!!!!!0 == 1);
	assert(!!!!!1 == 0);

	if (!(10)) {
		panic("");
	}

	if (!!10) {

	} else {
		panic("");
	}

	return 0;
}
