__declspec(dllimport) void assert(unsigned long long);

typedef struct {
	size_t b;
	int aa[2];
} Type;

// Since this function returns a struct, the first argument register will contain the address where
// the return struct sould be written, and the first function argument will be stored in the second
// argument register
Type new(int b) {
	return (Type) { .b = b };
}

int main(int argc, char* argv[]) {
	Type ty = new(88);
	assert(ty.b == 88);
	assert(ty.aa[0] == 0);
	assert(ty.aa[1] == 0);
	return 0;
}
