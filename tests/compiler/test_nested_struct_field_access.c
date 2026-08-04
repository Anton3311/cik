typedef struct {
	int value;
} Inner;

typedef struct {
	size_t a;
	Inner inner;
} Outer;

typedef struct {
	Inner first;
	Inner second;
} Inner2;

typedef struct {
	Inner2* first;
	Inner second;
	Inner* third;
} Outer2;

__declspec(dllimport) void assert(unsigned long long);
__declspec(dllimport) void* malloc(size_t);
__declspec(dllimport) void free(void*);

int main(int argc, char *argv[]) {
	Outer outer;
	outer.a = 10;
	outer.inner.value = -1;

	assert(outer.a == 10);
	assert(outer.inner.value == -1);

	{
		Outer2 o;
		o.first = (Inner2*)malloc(sizeof(Inner2));
		o.third = (Inner*)malloc(sizeof(Inner));
		o.first->first.value = 1;
		o.first->second.value = 2;
		o.second.value = 6;
		o.third->value = 7;

		assert(o.first->first.value == 1);
		assert(o.first->second.value == 2);
		assert(o.second.value == 6);
		assert(o.third->value == 7);

		free(o.third);
		free(o.first);
	}

	return 0;
}
