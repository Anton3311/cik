__declspec(dllimport) void assert(unsigned long long);

typedef struct {
	int value;
} Type;

int main(int argc, char *argv[]) {
	Type array[2];
	array[0].value = 0;
	array[1].value = 0;

	Type a;
	a.value = 88;

	array[0] = a;

	assert(array[0].value == 88);
	assert(array[1].value == 0);

	return 0;
}
