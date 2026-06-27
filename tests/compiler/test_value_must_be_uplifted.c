typedef unsigned long long uint64_t;

uint64_t identity(uint64_t);
void assert(uint64_t);

int main(int argc, char* argv[]) {
	uint64_t cond = 10;
	uint64_t value = 5;
	uint64_t result = 0;

	if (cond == 4) {
		result = value;
	} else {
		result = 2;
	}

	result = result + value;
	assert(result == 7);
	return 0;
}
