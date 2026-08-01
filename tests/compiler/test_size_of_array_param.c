__declspec(dllimport) void assert(unsigned long long);

int main(int argc, char *argv[]) {
	assert(sizeof(argv) == sizeof(char**));
	return 0;
}
