__declspec(dllimport) void assert(unsigned long long);

typedef struct {
	size_t a;
	size_t b;
} Compound;

int main(int argc, char *argv[]) {
	Compound a;
	a.a = 10;
	a.b = 88;

	Compound b = a;

	assert(a.a == 10);
	assert(a.b == 88);

	assert(b.a == 10);
	assert(b.b == 88);

	b.a = 99;
	b.b = 4;

	assert(a.a == 10);
	assert(a.b == 88);

	assert(b.a == 99);
	assert(b.b == 4);

	return 0;
}
