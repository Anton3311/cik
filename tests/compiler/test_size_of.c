__declspec(dllimport) void assert(unsigned long long);

typedef struct {

} EmptyStruct;

typedef struct {
	int a;
} Struct_4Bytes;

typedef struct {
	int a;
	long long b;
} Struct_16Bytes;

// Total size - 16
typedef struct {
	char a;  // offset - 0
	int b;   // offset - 4
	char c0; // offset - 8
	char c1; // offset - 9
	char c2; // offset - 10
	int d;   // offset - 12
} StructWithUselessPadding;

typedef struct {
	int b;   // offset - 0
	short d; // offset - 4
	char a;  // offset - 6
	char c;  // offset - 7
} StructWithoutUselessPadding;

typedef struct {
	unsigned long long big_int;
	int array[10];
} StructWithArray;

typedef union {
	int a;
} Union_4Bytes;

typedef union {
	int a;
	long long b;
} Union_8Bytes;

int main(int argc, char *argv[]) {
	// sizeof(<type>)
	assert(sizeof(char) == 1);
	assert(sizeof(signed char) == 1);
	assert(sizeof(unsigned char) == 1);

	assert(sizeof(short) == 2);
	assert(sizeof(signed short) == 2);
	assert(sizeof(unsigned short) == 2);

	assert(sizeof(int) == 4);
	assert(sizeof(signed int) == 4);
	assert(sizeof(unsigned int) == 4);

	assert(sizeof(long) == 4);
	assert(sizeof(signed long) == 4);
	assert(sizeof(unsigned long) == 4);

	assert(sizeof(long long) == 8);
	assert(sizeof(signed long long) == 8);
	assert(sizeof(unsigned long long) == 8);

	assert(sizeof(float) == 4);
	assert(sizeof(double) == 8);
	assert(sizeof(long double) == 8);

	assert(sizeof(size_t) == 8);
	assert(sizeof(void*) == 8);

	// Structs. sizeof without parenthesis
	assert(sizeof EmptyStruct == 1);

	assert(sizeof Struct_4Bytes == 4);
	assert(sizeof Struct_16Bytes == 16);

	assert(sizeof StructWithUselessPadding == 16);
	assert(sizeof StructWithoutUselessPadding == 8);

	assert(sizeof StructWithArray == 48);

	// Unions
	assert(sizeof(Union_4Bytes) == 4);
	assert(sizeof(Union_8Bytes) == 8);

	// sizeof(<expr>)
	assert(sizeof(0) == 4);

	assert(sizeof(0i8) == 1);
	assert(sizeof(0i16) == 2);
	assert(sizeof(0i32) == 4);
	assert(sizeof(0i64) == 8);

	assert(sizeof(1 == 1) == 4);

	Struct_16Bytes s_16_bytes;
	assert(sizeof(s_16_bytes) == sizeof(Struct_16Bytes));

	assert(sizeof(sizeof(0)) == 8);

	// With unary operator
	int a = 0;
	assert(sizeof a++ == 4);
	assert(sizeof a++ + 2 == 7);

	int b = 0;
	assert(sizeof --b == 4);
	assert(sizeof --b + 2 == 4);

	// With cast
	assert(sizeof((int)9) == sizeof(int));
	assert(sizeof((Union_8Bytes*)9) == sizeof(Union_8Bytes*));

	return 0;
}
