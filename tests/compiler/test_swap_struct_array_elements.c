__declspec(dllimport) void assert(unsigned long long);

typedef struct {
	int a;
} Type;

int main(int argc, char *argv[]) {
	Type array[2];
	array[0].a = 1;
	array[1].a = 2;

	assert(array[0].a == 1);
	assert(array[1].a == 2);

	Type temp = array[0];
	array[0] = array[1];
	array[1] = temp;

	assert(array[0].a == 2);
	assert(array[1].a == 1);

	return 0;
}
