typedef struct {
	int a;
	short b;
	short c;
} Inner;

typedef struct {
	int a;
	size_t b;
	short last;	
	Inner inner;
} A;

__declspec(dllimport) void assert(unsigned long long);

int main(int argc, char* argv[]) {
	A type = (A) { 9, .b = 100, .inner = (Inner) { 8, 6, 5 } };

	assert(type.a == 9);
	assert(type.b == 100);
	assert(type.last == 0i16);

	assert(type.inner.a == 8);
	assert(type.inner.b == 6i16);
	assert(type.inner.c == 5i16);

	return 0;
}
