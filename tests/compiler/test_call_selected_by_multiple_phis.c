typedef unsigned long long uint64_t;

uint64_t identity(uint64_t);
void assert(uint64_t);

int main(int argc, char* argv[]) {
	uint64_t primary = 10;
	uint64_t secondary = 0;
	uint64_t result = identity(5);
	if (primary == 10) {
		if (secondary == 99) {
			result = 8;
		}
	} else {
		if (secondary == 0) {
			result = 2;
		}
	}

	assert(result == 5);
	return 0;
}
