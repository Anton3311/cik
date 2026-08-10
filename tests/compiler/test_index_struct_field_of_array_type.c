__declspec(dllimport) void assert(unsigned long long);

typedef struct {
	int array[2];
} Type;

int main(int argc, char* argv[]) {
	Type ty = {};
	ty.array[0] = 99;

	assert(ty.array[0] == 99);
	assert(ty.array[1] == 0);
	return 0;
}
