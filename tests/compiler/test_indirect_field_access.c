#include <stdint.h>

__declspec(dllimport) void* malloc(size_t);
__declspec(dllimport) void free(void*);
__declspec(dllimport) void assert(uint64_t);

typedef struct Type Type;

struct Type {
	int a;
	int b;
	int c;
};

int main(int argc, char* argv[]) {
	Type* type = malloc(4 * 3);

	type->a = 0;
	type->b = 4;
	type->c = 88;

	assert(type->a == 0);
	assert(type->b == 4);
	assert(type->c == 88);

	free(type);
	return 0;
}
