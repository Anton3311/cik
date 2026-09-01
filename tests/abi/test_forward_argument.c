__declspec(dllimport) void assert(unsigned long long);

typedef struct {
	size_t a;
	size_t b;
} Struct;

void func1(Struct a) {
	assert(a.a == 1);
	assert(a.b == 2);
}

void func2(Struct a) {
	func1(a);
}

void verify_8(int a, int b, int c, int d,
		      int e, int f, int g, int h) {
	assert(a == 0);
	assert(b == 1);
	assert(c == 2);
	assert(d == 3);
	assert(e == 4);
	assert(f == 5);
	assert(g == 6);
	assert(h == 7);
}

void forward_8(int a, int b, int c, int d,
		       int e, int f, int g, int h) {
	verify_8(a, b, c, d, e, f, g, h);
}

void forward_reversed_8(int a, int b, int c, int d,
		                int e, int f, int g, int h) {
	verify_8(h, g, f, e, d, c, b, a);
}

int main(int argc, char *argv[]) {
	func2((Struct) { .a = 1, .b = 2 });
	forward_8(0, 1, 2, 3, 4, 5, 6, 7);
	forward_reversed_8(7, 6, 5, 4, 3, 2, 1, 0);
	return 0;
}
