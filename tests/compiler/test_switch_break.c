__declspec(dllimport) void assert(unsigned long long);

int map(int a) {
	int result = 0;
	switch (a) {
	case 0:
		result = 66;
		break;
	case 10:
		result = 44;
		break;
	default:
		result = 77;
	case 4:
		if (result == 77) {
			result = -9;
		} else {
			result = -3;
		}

		break;
	}

	return result;
}

int main(int argc, char *argv[]) {
	assert(map(0) == 66);
	assert(map(10) == 44);
	assert(map(1) == -9);
	assert(map(2) == -9);
	assert(map(3) == -9);
	assert(map(5) == -9);
	assert(map(6) == -9);
	assert(map(7) == -9);
	assert(map(8) == -9);
	assert(map(9) == -9);
	assert(map(4) == -3);
	return 0;
}
