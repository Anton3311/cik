__declspec(dllimport) void assert(unsigned long long);

int main(int argc, char* argv[]) {
	if (10 == 10 || 9 == 9) {
		assert(1);
	} else {
		assert(0);
	}

	if (10 == 10 || 9 == 6) {
		assert(1);
	} else {
		assert(0);
	}

	if (10 == 1 || 9 == 6) {
		assert(0);
	} else {
		assert(1);
	}

	return 0;
}
