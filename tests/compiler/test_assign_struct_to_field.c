__declspec(dllimport) void assert(unsigned long long);
__declspec(dllimport) int printf(const char*, ...);

typedef struct {
	int value;
	size_t _;
} Value;

typedef struct {
	Value direct;
} Type;

int main(int argc, char *argv[]) {
	Value zero;
	zero.value = 0;

	Type t;
	t.direct = zero;

	assert(&t.direct != &zero);
	assert(t.direct.value == 0);
	assert(zero.value == 0);

	t.direct.value = 10;
	assert(t.direct.value == 10);
	assert(zero.value == 0);

	Type t2;
	Type* t_ptr = &t2;
	t_ptr->direct = zero;

	assert(&t_ptr->direct != &zero);
	assert(t_ptr->direct.value == 0);
	assert(zero.value == 0);

	t_ptr->direct.value = 10;
	assert(t_ptr->direct.value == 10);
	assert(zero.value == 0);

	return 0;
}
