__declspec(dllimport) void assert(unsigned long long);

void test_sized_array_param(int values[10]) {

}

int main(int argc, char *argv[4]) {
	assert(sizeof(argv) == sizeof(char**));
	return 0;
}
