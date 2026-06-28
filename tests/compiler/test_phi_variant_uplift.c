typedef unsigned long long uint64_t;

uint64_t identity(uint64_t);
void assert(uint64_t);

int main(int argc, char* argv[]) {
	uint64_t primary = 10;
	uint64_t secondary = 99;
	uint64_t result = 4;

	if (primary == 10) {
		if (secondary == 99) {
			result = 8;

			uint64_t s = secondary + primary;
			if (s == 109) {
				result = result + 2;
			}
		}
	} else {
		if (secondary == 0) {
			result = 2;
		}
	}

	assert(result == 10);
	return 0;
}
