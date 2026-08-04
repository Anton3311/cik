__declspec(dllimport) void assert(unsigned long long);
__declspec(dllimport) int printf(const char*, ...);

#define check(c) if(!(c)) { printf("%d\n", __LINE__); assert(0);}

typedef struct {
	size_t a;
	int b;
	short c;
	short d;
	char e;
} Type;

int main(int argc, char *argv[]) {
	Type a;
	a.a = 1;
	a.b = 2;
	a.c = 3;
	a.d = 4;
	a.e = 5;

	Type b;
	b = a;

	check(a.a == b.a);
	check(a.b == b.b);
	check(a.c == b.c);
	check(a.d == b.d);
	check(a.e == b.e);

	b.a = 101;

	check(a.a == 1);
	check(b.a == 101);

	b.d = 55;

	check(a.d == 4i16);
	check(b.d == 55i16);

	return 0;
}
