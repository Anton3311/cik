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

int main(int argc, char *argv[]) {
	func2((Struct) { .a = 1, .b = 2 });
	return 0;
}
