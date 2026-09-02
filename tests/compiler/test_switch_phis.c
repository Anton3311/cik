__declspec(dllimport) void assert(unsigned long long);

int sum_cases(int a) {
	int result = 0;
	switch (a) {
	case 1:
		result += 1;
	case 2:
		result += 2;
	case 3:
		result += 3;
	}

	return result;
}

int main(int argc, char *argv[]) {
	assert(sum_cases(0) == 0);
	assert(sum_cases(1) == 6);
	assert(sum_cases(2) == 5);
	assert(sum_cases(3) == 3);
	assert(sum_cases(4) == 0);
	return 0;
}
