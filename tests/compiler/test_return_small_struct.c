// This a test for returning small structs, that are smaller than a register.

#include <stdio.h>

__declspec(dllimport) void assert(unsigned long long);

typedef struct {
	size_t a;
} RegisterSized;

typedef struct {
	char a;
} Bytes1;

typedef struct {
	short a;
} Bytes2;

typedef struct {
	short a;
	char b;
} Bytes3;

RegisterSized new_register_sized() {
	return (RegisterSized) { 0xdeadbeefdeadbeef };
}

Bytes1 new_bytes_1() {
	return (Bytes1) { 0xde };
}

Bytes2 new_bytes_2() {
	return (Bytes2) { 0xbeefui16 };
}

Bytes3 new_bytes_3() {
	return (Bytes3) { 0xdeadui16, 0xbe };
}

int main(int argc, char *argv[]) {
	printf("#0\n");
	RegisterSized reg_sized = new_register_sized();
	assert(reg_sized.a == 0xdeadbeefdeadbeef);

	printf("#1\n");
	Bytes1 bytes1 = new_bytes_1();
	assert(bytes1.a == 0xde);

	printf("#2\n");

	Bytes2 bytes2 = new_bytes_2();
	printf("%u", (unsigned int)bytes2.a);
	assert(bytes2.a == 0xbeefui16);

	printf("#3\n");
	Bytes3 bytes3 = new_bytes_3();
	assert(bytes3.a == 0xdeadui16);
	assert(bytes3.b == 0xbe);
	return 0;
}
