__declspec(dllimport) void* malloc(size_t);
__declspec(dllimport) void free(void*);
__declspec(dllimport) void assert(unsigned long long);

typedef struct {
	int* array;
} Type;

int main(int argc, char *argv[]) {
	Type ty = { .array = malloc(sizeof(int) * 2) };

	ty.array[0] = 10;
	ty.array[1] = 55;

	assert(ty.array[0] == 10);
	assert(ty.array[1] == 55);

	free(ty.array);
	return 0;
}
