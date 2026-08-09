__declspec(dllimport) void assert(unsigned long long);

typedef struct {
	int a;
	size_t b;
} Type;

int main(int argc, char* argv[]) {
	Type t = (Type) {};

	assert(t.a == 0);
	assert(t.b == 0);

	return 0;
}
