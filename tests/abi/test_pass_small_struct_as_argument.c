#include <stdio.h>

__declspec(dllimport) void assert(unsigned long long);

typedef struct Id {
	unsigned int value;
} Id;

Id id_next(Id id) {
	return (Id) { .value = id.value + 1 };
}

int main(int argc, char *argv[]) {
	Id a = id_next((Id) { 0 });
	printf("%u\n", a.value);
	assert(a.value == 1);
	Id b = id_next((Id) { 0xffffffff });
	printf("%u\n", b.value);
	assert(b.value == 0);
	return 0;
}
