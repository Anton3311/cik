__declspec(dllimport) void assert(unsigned long long);

typedef struct {
	size_t b;
	int aa[2];
} Type;

Type new() {
	return (Type) { .b = 88 };
}

int main(int argc, char* argv[]) {
	Type ty = new();
	assert(ty.b == 88);
	assert(ty.aa[0] == 0);
	assert(ty.aa[1] == 0);
	return 0;
}
