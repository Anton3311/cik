__declspec(dllimport) void assert(unsigned long long);

void modify_int(int* a) {
	*a = 999;
}

int main(int argc, char *argv[]) {
	int array[10];

	array[0] = 10;
	array[1] = 99;
	assert(array[0] == 10);
	assert(array[1] == 99);

	modify_int(array);

	assert(array[0] == 999);
	return 0;
}
