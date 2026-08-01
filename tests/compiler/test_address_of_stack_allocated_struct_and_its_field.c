#include <stdint.h>

__declspec(dllimport) void assert(uint64_t);

typedef struct {
	int a;
} Hello;

void modify_int(int* a) {
	*a = 999;
}

void modify(Hello* h) {
	h->a = 100;
}

int main(int argc, char* argv[]) {
	Hello a;
	a.a = 10;

	assert(a.a == 10);

	modify(&a);

	assert(a.a == 100);

	modify_int(&a.a);

	assert(a.a == 999);

	return 0;
}
