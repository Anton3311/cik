typedef struct {
	int a;
	size_t b;
	short last;	
} A;

__declspec(dllimport) void assert(unsigned long long);

int main(int argc, char* argv[]) {
	A type = { 9, .b = 100 };

	assert(type.a == 9);
	assert(type.b == 100);
	assert(type.last == 0i16);

	return 0;
}
