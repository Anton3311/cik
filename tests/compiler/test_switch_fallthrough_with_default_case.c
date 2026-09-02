__declspec(dllimport) void assert(unsigned long long);

int fallthrough_1(int a) {
	int result = 0;
	switch (a) {
	case 0:
		result = 10;
		break;
	default:
		result = 11;
	}

	return result;
}

int fallthrough_2(int a) {
	int result = 0;
	switch (a) {
	default:
		result = 2;
		break;
	case 0:
		result = 3;
	}

	return result;
}

int main(int argc, char *argv[]) {
	assert(fallthrough_1(0) == 10);
	assert(fallthrough_1(1) == 11);

	assert(fallthrough_2(0) == 3);
	assert(fallthrough_2(1) == 2);
	return 0;
}
