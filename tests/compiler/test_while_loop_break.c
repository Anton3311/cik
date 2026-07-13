#include <stdint.h>

void panic(const char*);

int main(int argc, char* argv[]) {
	int i = 0;
	while (i < 20) {
		break;

		panic("Unreachable code");
	}

	return 0;
}
