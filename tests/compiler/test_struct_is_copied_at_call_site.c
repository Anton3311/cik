__declspec(dllimport) void assert(unsigned long long);

typedef struct {
	size_t a;
	size_t b;
} Type;

void mutate_struct(Type type) {
	type.a = 99;
	type.b = 44;
}

int main(int argc, char* argv[]) {
	Type t = { 11, 22 };
	mutate_struct(t);
	assert(t.a == 11);
	assert(t.b == 22);
	return 0;
}
