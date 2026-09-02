__declspec(dllimport) void assert(unsigned long long);
__declspec(dllimport) void panic(const char*);

void default_case(int a) {
	switch (a) {
	case 0:
	case 1:
	case 2:
		assert(0);
	default:
		assert(1);
		return;
	}

	panic("default case not handled");
}

int main(int argc, char *argv[]) {
	default_case(3);
	return 0;
}
