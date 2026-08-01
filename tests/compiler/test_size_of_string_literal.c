__declspec(dllimport) void assert(unsigned long long);

int main(int argc, char *argv[]) {
	// The size includes the null terminator
	assert(sizeof "" == 1);
	assert(sizeof "hello world" == 12);
	return 0;
}
